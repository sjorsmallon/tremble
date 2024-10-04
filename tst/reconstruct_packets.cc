
#include <print>


#include "../src/packet.hpp"
#include "../src/concepts.hpp"
#include <cassert>


struct Position
{
    float x;
    float y;
    float z;
};

static void podvector_test()
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


    std::vector<Packet> packets = convert_to_packets(positions);

    // for the reconstruct test.    
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
static void pod_test()
{
	
}

int main()
{



}