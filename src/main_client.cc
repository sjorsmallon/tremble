#include <vector>
#include <string>
#include <numeric>
#include <cassert>
#include <thread>
#include "udp_socket.h"

#include "globals.hpp"
#include "server_config.hpp"
#include "concepts.hpp" // podvector
#include "packet.hpp"


struct Position
{
    float x;
    float y;
    float z;
};


int main(int argc, char* argv[])
{

    UDPsocket client{};
    client.open();
    client.broadcast(true);

    uint8_t start = 0;

    PodVector<Position> positions{};
    positions.resize(2048);
    for (int idx = 0; idx != 2048; ++idx)
    {
        auto& position = positions[idx];
        position.x = idx;
        position.y = idx;
        position.z = idx;
    }

    std::print("max buffer size:{}\n", MAX_BUFFER_SIZE_IN_BYTES);
    std::print("vec3 byte size: :{}\n", sizeof(Position));
    std::print("total data size: {}\n", sizeof(Position) * positions.size());

    std::vector<Packet> packets = convert_to_packets(positions);
    std::print("sending {} packets.\n", packets.size());

    // auto t1 = std::thread([&]{
        for (auto& packet: packets)
        {
            if (client.send(packet, UDPsocket::IPv4::Broadcast(global::port_number)) < 0)
            {
                std::print("send(): failed (REQ)\n");
            }
            else 
            {
                std::print("send message succeeded.\n");
            }
            std::this_thread::sleep_for(200ms);
        }
    // });

    // t1.join();

    
   client.close();

   //reconstruct!
    auto buffer = std::vector<uint8_t>{}; // 1mb?
    buffer.resize(std::size_t{2048 * 2048});

    auto byte_offset_from_start = 0;
    for (auto& packet: packets)
    {
        auto& packet_header = packet.header;
        std::print("total packets in sequence: {}\n", packet_header.sequence_count);
        // are we part of a sequence?
        if (packet_header.sequence_id > 0)
        {
            assert(packet_header.sequence_count >= packet_header.sequence_idx);
            // insert the packet data in the right place.
            // just write it out.
            std::memcpy(&buffer[byte_offset_from_start], &packet.buffer, packet_header.payload_size);
            byte_offset_from_start += packet_header.payload_size;
        }
    }

    Position* reconstructed_positions = reinterpret_cast<Position*>(buffer.data());
    for (int idx = 0; idx != 2048; ++idx)
    {
        std::print("idx: {},position.x:{}, position.y: {}, position.z: {}\n",idx, reconstructed_positions[idx].x, reconstructed_positions[idx].y, reconstructed_positions[idx].z);
        assert(static_cast<int>( reconstructed_positions[idx].x) == idx);
        assert(static_cast<int>( reconstructed_positions[idx].y) == idx);
        assert(static_cast<int>( reconstructed_positions[idx].z) == idx);

    }


}