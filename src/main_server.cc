#include "udp_socket.h"
#include <thread>
#include <chrono>
#include <string>

#include "globals.hpp"
#include "server_config.hpp"
using namespace std::chrono_literals;
using namespace std::literals;

int main()
{

	auto t1 = std::thread([]
	{
		UDPsocket server_socket{};
		server_socket.open();
		server_socket.bind(global::port_number);

		std::print("server open and bound on port {}\n", global::port_number);
		while (true)
		{
			UDPsocket::IPv4 ipaddr{};
			std::string data;
			if (server_socket.recv(data, ipaddr) < 0) // blocking
			{
				fprintf(stderr, "recv(): failed\n");
			}
			else
			{
				std::print("ip {} sent {}\n",ipaddr.to_string().c_str(),data.c_str());
			}
		}
		server_socket.close();
	});

	// 	for (uint16_t i = 0; i < 2 * global::iteration_count; ++i)
	// 	{
	// 		UDPsocket::IPv4 ipaddr{};
	// 		std::string data;
	// 		if (server_socket.recv(data, ipaddr) < 0) // blocking
	// 		{
	// 			fprintf(stderr, "recv(): failed\n");
	// 		}
	// 		else
	// 		{
	// 			if (!data.empty())
	// 			{
	// 				std::print("ip {} sent {}", ipaddr.to_string().c_str(),data.c_str());
	// 				if (data.compare(0, 8, "MESSAGE?"s) == 0)
	// 				{
	// 					ipaddr.port = global::port_number;
	// 					if (i & 0x2)
	// 					{

	// 					}
	// 					else
	// 					{
	// 						if (server_socket.send("MESSAGE!"s, ipaddr) < 0)
	// 						{
	// 							fprintf(stderr, "send(): failed (REP)\n");
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// 	server_socket.close();
	// });


	t1.join();

}