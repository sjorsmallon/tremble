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
        }
    // });

    // t1.join();

    
   client.close();
}