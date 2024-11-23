#pragma once

constexpr auto sv_max_player_count = 32;
constexpr auto server_port_number = 2020;
constexpr auto client_port_number = 2024;


struct Byte_Buffer
{
	std::vector<uint8_t> data = std::vector<uint8_t>(2048 * 2048);
	size_t cursor = 0; // byte_offset to insert at.
};


// are players unique enough?
struct Player_Movement_State
{	
	vec3 position;
	vec3 front;
	vec3 velocity;
};

struct Server_Connection_State
{
	// things we thought about
	std::array<bool, sv_max_player_count> player_slots{};
	std::array<UDPsocket::IPv4, sv_max_player_count> player_ips{};
	std::array<Byte_Buffer, sv_max_player_count> player_byte_buffers{};

	// things that need refinement (this is gameplay related)
	std::array<Player_Movement_State, sv_max_player_count> player_movement_states{};

};

inline void disconnect_player(Server_Connection_State& server_connection_state, const UDPsocket::IPv4 ip)
{
	int idx = 0;
	for(auto& player_ip: server_connection_state.player_ips)
	{
		if (ip == player_ip)
		{
			server_connection_state.player_ips[idx] = {};
			server_connection_state.player_slots[idx] = false;
			server_connection_state.player_movement_states[idx] = {};

			return;
		}
		idx += 1;
	}
}

// can return null, i guess. // server player state is not const because I am returning a non-const pointer from it.
inline Byte_Buffer* get_player_packet_byte_buffer_from_ip(Server_Connection_State& server_connection_state, const UDPsocket::IPv4 ip)
{
	int idx = 0;
	for(auto& player_ip: server_connection_state.player_ips)
	{
		if (ip == player_ip) return &server_connection_state.player_byte_buffers[idx];
		idx += 1;
	}

	return nullptr;
}

inline size_t get_player_idx(Server_Connection_State& server_connection_state, const UDPsocket::IPv4 ip)
{
	size_t idx = 0;
	for(auto& player_ip: server_connection_state.player_ips)
	{
		if (ip == player_ip)
		{
			return idx;
		}
		idx += 1;
	}

	print_warning("player not found while get_player_idx is invoked...\n");
	return -1;
}

inline Player_Movement_State& get_player_movement_state(Server_Connection_State& server_connection_state, size_t idx)
{
	assert(idx < sv_max_player_count);

	return server_connection_state.player_movement_states[idx];
}
