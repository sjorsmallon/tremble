#include "../src/concepts.hpp"
#include "../src/udp_socket.hpp"

#include <print>
#include <cassert>
#include <thread>
#include <chrono>

struct Position
{
	float x;
	float y;
	float z;
};

constexpr auto port_number = 2020;


int main()
{
	PodVector<Position> positions{};
    positions.resize(2048);
    for (int idx = 0; idx != 2048; ++idx)
    {
        auto& position = positions[idx];
        position.x = idx;
        position.y = idx;
        position.z = idx;
	}

    auto t2 = std::thread([&]{
		UDPsocket client{};
	    client.open();
	    client.broadcast(true);

	    std::vector<Packet> packets = convert_to_packets(positions, MESSAGE_JOIN_SERVER);

        for (auto& packet: packets)
        {
            if (client.send(packet, UDPsocket::IPv4::Broadcast(port_number)) < 0)
            {
                std::print("send(): failed (REQ)\n");
            }
            else 
            {
                std::print("send message succeeded.\n");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
   		client.close();
	 });


	auto t1_buffer = std::vector<uint8_t>{}; // 1mb?
	t1_buffer.resize(std::size_t{2048 * 2048});

	auto t1 = std::thread([&]
	{
		UDPsocket server_socket{};
		server_socket.open();
		server_socket.bind(port_number);

		std::print("server open and bound on port {}\n", port_number);

		// the t1_buffer will be the reconstructed contiguous representation of the messages.
		// this means that if messages arrive OoO, we will have segments of the t1_buffer
		// that are still zero.
		// TODO: signal when we are complete.
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
				auto& packet_header = packet.header;
				
				std::print("total packets in sequence: {}\n", packet_header.sequence_count);
				// are we part of a sequence?
				if (packet_header.sequence_id > 0)
				{
					assert(packet_header.sequence_count >= packet_header.sequence_idx);
					std::memcpy(&t1_buffer[byte_offset_from_start], &packet.buffer, packet_header.payload_size);
					byte_offset_from_start += packet_header.payload_size;
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

		server_socket.close();
	});

	t1.join();
	t2.join();

	Position* received_positions = reinterpret_cast<Position*>(t1_buffer.data());

	for (int idx = 0; idx != 2048; ++idx)
	{
		std::print("idx: {},position.x:{}, position.y: {}, position.z: {}\n",idx, received_positions[idx].x, received_positions[idx].y, received_positions[idx].z);
		assert(static_cast<int>( received_positions[idx].x) == idx);
		assert(static_cast<int>( received_positions[idx].y) == idx);
		assert(static_cast<int>( received_positions[idx].z) == idx);
	}

}