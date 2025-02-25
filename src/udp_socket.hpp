#pragma once
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifndef INPORT_ANY
#define INPORT_ANY 0
#endif

#include <cstring>
#include <array>
#include <string>
#include <vector>

#include "packet.hpp"


//@NOTE(SJM): this is lifted from https://github.com/barczynsky/UDPsocket.
// it is adapted to not use a string but a packet.

class UDPsocket
{
public:
	typedef struct sockaddr_in sockaddr_in_t;
	typedef struct sockaddr sockaddr_t;
	typedef std::vector<uint8_t> msg_t;

public:
	struct IPv4;

	enum class Status : int
	{
		OK = 0,
		SocketError = -1,
		OpenError = SocketError,
		CloseError = -2,
		ShutdownError = -3,
		BindError = -4,
		ConnectError = BindError,
		SetSockOptError = -5,
		GetSockNameError = -6,
		SendError = -7,
		RecvError = -8,
		// AddressError = -66,
	};

private:
	int sock{ -1 };
	sockaddr_in_t self_addr{};
	socklen_t self_addr_len = sizeof(self_addr);
	sockaddr_in_t peer_addr{};
	socklen_t peer_addr_len = sizeof(peer_addr);

public:
	UDPsocket()
	{
#ifdef _WIN32
		WSAInit();
#endif
		self_addr = IPv4{};
		peer_addr = IPv4{};
	}

	~UDPsocket()
	{
		this->close();
	}

public:
	int open()
	{
		this->close();
		sock = (int)::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);



		if (this->is_closed()) {
			return (int)Status::SocketError;
		}
		// make nonblocking.
		this->set_nonblocking(sock);


		return (int)Status::OK;
	}

	int close()
	{
		if (!this->is_closed()) {
#ifdef _WIN32
			int ret = ::shutdown(sock, SD_BOTH);
#else
			int ret = ::shutdown(sock, SHUT_RDWR);
#endif
			if (ret < 0) {
				return (int)Status::ShutdownError;
			}
#ifdef _WIN32
			ret = ::closesocket(sock);
#else
			ret = ::close(sock);
#endif
			if (ret < 0) {
				return (int)Status::CloseError;
			}
			sock = -1;
		}
		return (int)Status::OK;
	}

	bool is_closed() const { return sock < 0; }

public:
	int bind(const IPv4& ipaddr)
	{
		self_addr = ipaddr;
		self_addr_len = sizeof(self_addr);
		int opt = 1;
		int ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
		if (ret < 0) {
			return (int)Status::SetSockOptError;
		}
		ret = ::bind(sock, (sockaddr_t*)&self_addr, self_addr_len);
		if (ret < 0) {
			return (int)Status::BindError;
		}
		ret = ::getsockname(sock, (sockaddr_t*)&self_addr, &self_addr_len);
		if (ret < 0) {
			return (int)Status::GetSockNameError;
		}
		return (int)Status::OK;
	}

	void set_nonblocking(int sock) {
	#ifdef _WIN32
	    u_long mode = 1;
	    ioctlsocket(sock, FIONBIO, &mode);
	#else
	    int flags = fcntl(sock, F_GETFL, 0);
	    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	#endif
	}

	int bind(uint16_t portno)
	{
		auto ipaddr = IPv4::Any(portno);
		return this->bind(ipaddr);
	}

	int bind_any()
	{
		return this->bind(INPORT_ANY);
	}

	int bind_any(uint16_t& portno)
	{
		int ret = this->bind(INPORT_ANY);
		if (ret < 0) {
			return ret;
		}
		portno = IPv4{ self_addr }.port;
		return (int)Status::OK;
	}

public:
	int connect(const IPv4& ipaddr)
	{
		peer_addr = ipaddr;
		peer_addr_len = sizeof(peer_addr);
		int ret = ::connect(sock, (sockaddr_t*)&peer_addr, peer_addr_len);
		if (ret < 0) {
			return (int)Status::ConnectError;
		}
		return (int)Status::OK;
	}

	int connect(uint16_t portno)
	{
		auto ipaddr = IPv4::Loopback(portno);
		return this->connect(ipaddr);
	}

public:
	IPv4 get_self_ip() const
	{
		return self_addr;
	}

	IPv4 get_peer_ip() const
	{
		return peer_addr;
	}

public:
	
	int send(const Packet& message, const IPv4& ipaddr) const
	{
		// // UPnP
		// std::string msg = "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nMAN: ssockp:discover\r\nST: ssockp:all\r\nMX: 1\r\n\r\n";
		sockaddr_in_t addr_in = ipaddr;
		socklen_t addr_in_len = sizeof(addr_in);
		int ret = ::sendto(sock,
			(const char*)&message, sizeof(message), 0,
			(sockaddr_t*)&addr_in, addr_in_len);
		if (ret < 0) {
			return (int)Status::SendError;
		}
		return ret;
	}

	// template <typename T, typename = typename
	// 	std::enable_if<sizeof(typename T::value_type) == sizeof(uint8_t)>::type>
	int recv(Packet& message, IPv4& ipaddr) const
	{
		sockaddr_in_t addr_in;
		socklen_t addr_in_len = sizeof(addr_in);
		// typename T::value_type buffer[10 * 1024];
		Packet destination; 
		int ret = ::recvfrom(sock,
			(char*)&destination, sizeof(destination), 0,
			(sockaddr_t*)&addr_in, &addr_in_len);
		
		if (ret < 0)
		{
			return (int)Status::RecvError;
		}
		ipaddr = addr_in;
		// message = { std::begin(buffer), std::begin(buffer) + ret };
		message = destination;
		return ret;
	}


	int recv_nonblocking_but_try_every_so_often(Packet& message, IPv4& ipaddr, const int max_wait_time_ms, const int check_interval_ms)
	{
	    int elapsed_time = 0;
	    int result{};

	    while (elapsed_time < max_wait_time_ms)
		{
			result = this->recv(message, ipaddr);
			if (result > 0) break;

		    // Wait for 10ms before the next check
		    std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
		    elapsed_time += check_interval_ms;
		}

		return result;
	}



public:
	int broadcast(int opt) const
	{
		int ret = ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt));
		if (ret < 0) {
			return (int)Status::SetSockOptError;
		}
		return (int)Status::OK;
	}

	int interrupt() const
	{
		uint16_t portno = IPv4{ self_addr }.port;
		auto ipaddr = IPv4::Loopback(portno);
		return this->send(Packet{}, ipaddr);
	}

public:
	struct IPv4
	{
		std::array<uint8_t, 4> octets{};
		uint16_t port{};

	public:
		IPv4()
		{
		}

		IPv4(const std::string& ipaddr, uint16_t portno)
		{
			int ret = ::inet_pton(AF_INET, ipaddr.c_str(), (uint32_t*)octets.data());
			if (ret > 0) {
				port = portno;
			} else {
				// throw std::runtime_error(Status::AddressError)
			}
		}

		IPv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t portno)
		{
			octets[0] = a;
			octets[1] = b;
			octets[2] = c;
			octets[3] = d;
			port = portno;
		}

		IPv4(const sockaddr_in_t& addr_in)
		{
			*(uint32_t*)octets.data() = addr_in.sin_addr.s_addr;
			port = ntohs(addr_in.sin_port);
		}

		operator sockaddr_in_t() const
		{
			sockaddr_in_t addr_in;
			std::memset(&addr_in, 0, sizeof(addr_in));
			addr_in.sin_family = AF_INET;
			addr_in.sin_addr.s_addr = *(uint32_t*)octets.data();
			addr_in.sin_port = htons(port);
			return addr_in;
		}

	private:
		IPv4(uint32_t ipaddr, uint16_t portno)
		{
			*(uint32_t*)octets.data() = htonl(ipaddr);
			port = portno;
		}

	public:
		static IPv4 Any(uint16_t portno) { return IPv4{ INADDR_ANY, portno }; }
		static IPv4 Loopback(uint16_t portno) { return IPv4{ INADDR_LOOPBACK, portno }; }
		static IPv4 Broadcast(uint16_t portno) { return IPv4{ INADDR_BROADCAST, portno }; }

	public:
		const uint8_t& operator[](size_t octet) const { return octets[octet]; }
		uint8_t& operator[](size_t octet) { return octets[octet]; }

	public:
		bool operator==(const IPv4& other) const {
			return this->octets == other.octets && this->port == other.port;
		}

		bool operator!=(const IPv4& other) const {
			return !(*this == other);
		}

	public:
		std::string addr_string() const {
			return std::to_string(octets[0]) +
			 '.' + std::to_string(octets[1]) +
			 '.' + std::to_string(octets[2]) +
			 '.' + std::to_string(octets[3]);
		}

		std::string port_string() const {
			return std::to_string(port);
		}

		std::string to_string() const {
			return this->addr_string() + ':' + this->port_string();
		}

		operator std::string() const { return this->to_string(); }
	};

#ifdef _WIN32
public:
	static WSADATA* WSAInit()
	{
		static WSADATA wsa;
		static struct WSAContext {
			WSAContext(WSADATA* wsa) {
				WSAStartup(0x0202, wsa);
			}
			~WSAContext() {
				WSACleanup();
			}
		} context{ &wsa };
		return &wsa;
	}
#endif
};

namespace std
{
    template<> struct hash<UDPsocket::IPv4>
    {
        typedef UDPsocket::IPv4 argument_type;
        typedef size_t result_type;
        result_type operator()(argument_type const& ipaddr) const noexcept
        {
            result_type const h1{ std::hash<uint32_t>{}(*(uint32_t*)ipaddr.octets.data()) };
            result_type const h2{ std::hash<uint16_t>{}(ipaddr.port) };
            return h1 ^ (h2 << 1);
        }
    };
}

// print stuff.
template <>
struct std::formatter<UDPsocket::IPv4> : std::formatter<std::string> {
    auto format(const UDPsocket::IPv4& ip, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}.{}.{}.{}:{}", ip.octets[0], ip.octets[1], ip.octets[2], ip.octets[3], ip.port);
    }
};


