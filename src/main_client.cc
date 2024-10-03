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

    PodVector<Position> positions{2048};
    for (int idx = 0; idx != 2048; ++idx)
    {
        auto& position = positions[idx];
        position.x = idx;
        position.y = idx;
        position.z = idx;
    }

    std::vector<Packet> packets = convert_to_packets(positions);
 
    // for (packet: packets)
    // {
    //     if (client.send(packet, UDPsocket::IPv4::Broadcast(global::port_number)) < 0)
    //     {
    //         fprintf(stderr, "send(): failed (REQ)\n");
    //     }
    //     else 
    //     {
    //         fprintf(stderr, "send message succeeded.\n");
    //     }
    
    // }
    
    // client.close();
}