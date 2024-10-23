#pragma once
#include <print>
#include "concepts.hpp" // pod

static_assert(__cplusplus == 202302L);

// none of this is formal. I just want to experiment.
constexpr std::size_t PLAYER_NAME_BUFFER_SIZE = 256;
constexpr std::size_t MESSAGE_BUFFER_SIZE = 2048; 

enum Message_Type : uint8_t
{
	MESSAGE_JOIN_SERVER,
	MESSAGE_LEAVE_SERVER,
	MESSAGE_CHAT_MSG,
	MESSAGE_ENTITY_DATA,
	// server messages?
	MESSAGE_JOIN_SERVER_ACCEPTED,
	MESSAGE_RECEIVED, // general ack for each packet?
	MESSAGE_MISSING_PACKET, // send back an index and a sequence?
	MESSAGE_PLAYER_INPUT
};

std::string to_string(Message_Type message_type)
{
	std::string result{};
	switch(message_type)
	{
		case MESSAGE_JOIN_SERVER :
		{
			result = "MESSAGE_JOIN_SERVER";
			break;
		}
		case MESSAGE_LEAVE_SERVER :
		{
			result = "MESSAGE_LEAVE_SERVER";
			break;
		}

		case MESSAGE_CHAT_MSG :
		{
			result = "MESSAGE_CHAT_MSG";
			break;
		}
		default:
			std::print("received invalid Message_Type. integer value {}. returning empty string.\n", static_cast<int>(message_type));
			break;
	}
	return result;
}



// the correct way to do this is to have some sort of base message that we inherit from.
// this means that we can embed the message type in there.
struct Base_Message
{
	Message_Type message_type;

	virtual ~Base_Message() = default;
};

struct Join_Server_Message : Base_Message
{
	std::string player_name;

	Join_Server_Message()
	{
		this->message_type = MESSAGE_JOIN_SERVER;
	}

	~Join_Server_Message() = default;
};

struct Chat_Message: Base_Message
{
	std::string message;
	Chat_Message()
	{
		this->message_type = MESSAGE_CHAT_MSG;
	}

	~Chat_Message() = default;
};


// the other way we can do this is by making sure that all messages start with the same "header":
struct Join_Server_Message_C
{
	Message_Type message_type = Message_Type::MESSAGE_JOIN_SERVER;
	uint8_t player_name_buffer[PLAYER_NAME_BUFFER_SIZE];
};


struct Leave_Server_Message_C
{
	Message_Type message_type = Message_Type::MESSAGE_LEAVE_SERVER;
	uint8_t player_name_buffer[PLAYER_NAME_BUFFER_SIZE];
};

struct Chat_Message_C
{
	Message_Type message_type = Message_Type::MESSAGE_CHAT_MSG;
	uint8_t message_buffer[MESSAGE_BUFFER_SIZE];
};

