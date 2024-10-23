#include <thread>
#include <chrono>
#include <string>
#include <cassert>

#include "../src/message.h"
#include "../src/udp_socket.h"

#include <memory>


using namespace std::chrono_literals;
using namespace std::literals;

static_assert(sizeof(Message_Type) == sizeof(uint8_t));

struct Position
{
    float x;
    float y;
    float z;
};


// global state that I do not know where to keep other than here for now.



static void process_message(Packet& packet, std::vector<uint8_t> buffer)
{
	switch(packet.header.message_type)
	{
		case MESSAGE_JOIN_SERVER :
		{
			break;
		}
		case MESSAGE_LEAVE_SERVER :
		{
			break;
		}
		case MESSAGE_ENTITY_DATA:
		{
			break;
		}
	}

}

constexpr auto sv_max_player_count = 32;
constexpr auto server_port_number = 2020;
constexpr auto client_port_number = 2024;


struct Byte_Buffer
{
	std::vector<uint8_t> data = std::vector<uint8_t>(2048 * 2048);
	size_t cursor = 0; // byte_offset to insert at.
};

struct Server_Player_State
{
	
	std::array<bool, sv_max_player_count> player_slots{};
	std::array<UDPsocket::IPv4, sv_max_player_count> player_ips{};
	std::array<Byte_Buffer, sv_max_player_count> player_byte_buffers{};
};

// can return null, i guess. // server player state is not const because I am returning a non-const pointer from it.
inline Byte_Buffer* get_player_byte_buffer_from_ip(Server_Player_State& server_state, const UDPsocket::IPv4 ip)
{
	int idx = 0;
	for(auto& player_ip: server_state.player_ips)
	{
		if (ip == player_ip) return &server_state.player_byte_buffers[idx];
		idx += 1;
	}

	return nullptr;
}


//@Note: this is mostly incomplete, and just me thinking about what this should look like.
// each  "player" has a scratchpad buffer, in which packets are stitched together into their original message.
// currently, there is no 'ack' sent for any data packet, just for the 'connect_to_server_message' packet.
// this is so we can sort of simulate 'connect to server' behavvior and only do things for those people.
// the buffer will be the reconstructed contiguous representation of the messages.
// this means that if messages arrive OoO, we will have segments of the buffer
// that are still zero. 

int main()
{
	//@FIXME: this is very wasteful. but for now, whatever.
	auto server_player_state = Server_Player_State{};

	auto t1 = std::thread([&]
	{
		UDPsocket server_socket{};
		server_socket.open();
		server_socket.bind(server_port_number);
		// also be able to send messages. I guess.
		server_socket.broadcast(true);

		std::print("[server] open and bound on port {}\n", server_port_number);


		auto packet_count = 0;
		auto byte_offset_from_start = 0;
		
		UDPsocket::IPv4 ipaddr{};
		while (true)
		{
			Packet packet{};
			if (server_socket.recv(packet, ipaddr) < 0) // blocking
			{
				fprintf(stderr, "recv(): failed\n");
			}
			else
			{	
				std::print ("[server] packet received.\n");
				Byte_Buffer* current_buffer_ptr = nullptr; // non-owning, just point to one.

				// who did we receive this from? do we know this guy?
				bool player_found = false;
				for (auto& player_ip: server_player_state.player_ips)
				{
					if (ipaddr == player_ip)
					{
						player_found = true;
					}
				}

				// if player is not found, but wants to connect:
				if (!player_found && packet.header.message_type == MESSAGE_JOIN_SERVER)
				{
					// i wanted to call this connect_player_to_server(player_ips, player_slots, ipaddr) but I only use it here so who cares
					// find the first empty slot
					for (int idx = 0; idx != sv_max_player_count; ++idx)
					{
						if (!server_player_state.player_slots[idx])
						{
							server_player_state.player_slots[idx] = true;
							server_player_state.player_ips[idx] = ipaddr;
							Packet join_server_accepted_packet = construct_message_only_packet(MESSAGE_JOIN_SERVER_ACCEPTED);

							//@FIXME: wait for one second so the message does not get lost.
							std::this_thread::sleep_for(std::chrono::seconds(1));
							server_socket.send(join_server_accepted_packet, UDPsocket::IPv4::Broadcast(client_port_number));
							break;
						}
					}
				}

				if (player_found)
				{
					current_buffer_ptr = get_player_byte_buffer_from_ip(server_player_state, ipaddr);
				}

				if (current_buffer_ptr == nullptr) std::print("[server] current_buffer_ptr == nullptr. that's not supposed to happen. crash incoming.\n");

				auto& buffer = current_buffer_ptr->data;

				auto& packet_header = packet.header;
				std::print("[server] total packets in sequence: {}\n", packet_header.sequence_count);
				// are we part of a sequence?
				if (packet_header.sequence_id > 0)
				{

					assert(packet_header.sequence_count >= packet_header.sequence_idx);
					std::memcpy(&buffer[byte_offset_from_start], &packet.buffer, packet_header.payload_size);
					//@FIXME: actually, we should be really careful. this just inserts packages one behind the other.
					// we need to reconstruct the actual payload.
					byte_offset_from_start += packet_header.payload_size; 

				}
				else
				{
					std::print("[server] one-shot packet received.\n");
				}

				// if this packet as part of a sequence, and the sequence is complete, hand it off to someone else.
				if (packet_header.sequence_id > 0 && packet_count == packet_header.sequence_count)
				{
					// copy the buffer and zero the buffer.
					auto handoff_buffer = std::vector<uint8_t>(byte_offset_from_start);
					std::memcpy(&handoff_buffer[0], &buffer[0], byte_offset_from_start);
					for(auto idx = 0; idx != byte_offset_from_start; ++idx)
					{
						buffer[idx] = uint8_t{0};
					}

					// process message (handover to different thread?)
					// process_message(packet, std::move(handoff_buffer));
				}
			}
		}

		server_socket.close();
	});


	auto t2 = std::thread([&](){
		UDPsocket client_socket{};
		client_socket.open();
		client_socket.bind(client_port_number);
		client_socket.broadcast(true);
		std::print("[client] started.{}\n", client_port_number);

		std::this_thread::sleep_for(std::chrono::seconds(3));
		std::print("[client] done sleeping.\n");
		Packet packet = construct_message_only_packet(MESSAGE_JOIN_SERVER);
		client_socket.send(packet, UDPsocket::IPv4::Broadcast(server_port_number));
		
		// wait for a response.
		UDPsocket::IPv4 ipaddr{};
		Packet incoming_packet{};
		if (client_socket.recv(incoming_packet, ipaddr) < 0) // blocking
		{
			fprintf(stderr, "recv(): failed\n");
		}
		else
		{
			std::print("[client] received a packet.\n");
		}
	});

	t2.join();
	t1.join();

}