#include "udp_socket.h"
#include <thread>
#include <chrono>
#include <string>
#include <cassert>

#include "globals.hpp"
#include "server_config.hpp"
#include "message.h"
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

int main()
{
	auto buffer = std::vector<uint8_t>{}; // 1mb?
	buffer.reserve(std::size_t{1024 * 1024});

	auto t1 = std::thread([&]
	{
		UDPsocket server_socket{};
		server_socket.open();
		server_socket.bind(global::port_number);

		std::print("server open and bound on port {}\n", global::port_number);

		// the buffer will be the reconstructed contiguous representation of the messages.
		// this means that if messages arrive OoO, we will have segments of the buffer
		// that are still zero.

		auto packet_count = 0;
		while (true)
		{
			UDPsocket::IPv4 ipaddr{};
			Packet packet{};
			if (server_socket.recv(packet, ipaddr) < 0) // blocking
			{
				fprintf(stderr, "recv(): failed\n");
			}
			else
			{	
				auto& packet_header = packet.header;
				
				std::print("total packets in sequence: {}\n", packet_header.sequence_count);
				// are we part of a sequence?
				if (packet_header.sequence_id > 0)
				{
					assert(packet_header.sequence_count >= packet_header.sequence_idx);
					// insert the packet data in the right place.
					// just write it out.
					auto byte_offset_from_start = packet_header.payload_size * packet_header.sequence_idx;
					std::memcpy(&buffer[byte_offset_from_start], &packet.buffer, packet_header.payload_size);

				}
				else
				{
					assert(false && "sequence id is zero.");
				}

				packet_count += 1;	
				std::print("total received packets: {}\n", packet_count);

				if (packet_count == packet_header.sequence_count) break;
			}
		}

		// just run this for 255 whatever



		server_socket.close();
	});

	t1.join();


	Position* positions = reinterpret_cast<Position*>(buffer.data());

	for (int idx = 0; idx != 255; ++idx)
	{
		std::print("position.x:{}, position.y: {}, position.z: {}\n", positions[idx].x, positions[idx].y, positions[idx].z);
	}

}