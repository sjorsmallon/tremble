#include "udp_socket.h"
#include <thread>
#include <chrono>
#include <string>

#include "globals.hpp"
#include "server_config.hpp"
#include "message.h"
#include <memory>
using namespace std::chrono_literals;
using namespace std::literals;

static_assert(sizeof(Message_Type) == sizeof(uint8_t));

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
				std::print("received a message. deconstructing..");
				Message_Type* message_type = reinterpret_cast<Message_Type*>(&data[0]);
				std::print("message type = {}\n", to_string(*message_type)); 

				// std::print("ip {} sent {}\n",ipaddr.to_string().c_str(), data.c_str());
			}
		}
		server_socket.close();
	});

	

	t1.join();

}