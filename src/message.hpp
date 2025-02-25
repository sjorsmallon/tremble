#pragma once
#include <print>
#include "concepts.hpp" // pod
#include  "player_move.hpp"

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
	MESSAGE_PLAYER_MOVE,
	MESSAGE_PROVIDE_CHALLENGE,
	MESSAGE_ANSWER_CHALLENGE,
	MESSAGE_CHANGE_MAP // user should load this map.
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

		case MESSAGE_PLAYER_MOVE: 
		{
			result = "MESSAGE_PLAYER_MOVE";
			break;
		}
		case MESSAGE_CHANGE_MAP:
		{
			result = "MESSAGE_CHANGE_MAP";
			break;
		}

		default:
			std::print("received invalid Message_Type. integer value {}. returning empty string.\n", static_cast<int>(message_type));
			break;
	}
	return result;
}

struct Player_Move_Message
{
	Move_Input move_input;
	vec3 front;
	vec3 right;
};


