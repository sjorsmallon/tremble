#include <thread>
#include <chrono>
#include <string>
#include <cassert>
#include <memory>

#include "../src/AABB.hpp"
#include "../src/bsp.hpp"
#include "../src/udp_socket.hpp"
#include "../src/vec.hpp"
#include "../src/player_move.hpp" // for Move_Input.
#include "../src/server.hpp"
#include <chrono>
 using namespace std::chrono;

static_assert(sizeof(Message_Type) == sizeof(uint8_t));


//@incomplete: what if a client tries to connect twice?
//@incomplete: buffer stitching is still sequential.
int main()
{
	// load the same map as we are loading on the client side.

    auto path = std::string{"../data/just_a_floor_AABBs"};
    auto aabbs = read_AABBs_from_file(path);
    auto map_vertices  = to_vertex_xnc(aabbs);
    BSP* map_bsp = build_bsp(map_vertices);

    auto player_collider_aabb = AABB{.min = vec3{-20.0f, -20.0f, -20.0f}, .max = {20.0f, 45.f, 20.f}};


	// vec3 player_position{-6.0320406, 10, 580.2726};

	auto t1 = std::thread([&] // to capture the server connection state, but that's actually not necessary?
	{
		auto server_connection_state = Server_Connection_State{};

		UDPsocket server_socket{};
		server_socket.open();
		server_socket.bind(server_port_number);

		// also be able to send messages. I guess.
		server_socket.broadcast(true);
		print_network("[server] open and bound on port {}\n", server_port_number);

		auto packet_count = 0;
		auto byte_offset_from_start = 0;
		
		UDPsocket::IPv4 ipaddr{};
		int total_packet_count = 0;

    	
    	auto last = high_resolution_clock::now();

		while (true)
		{
		
			Packet packet{};
			if (server_socket.recv(packet, ipaddr) < 0) // blocking
			{
				fprintf(stderr, "recv(): failed\n");
			}
			else
			{	
				if (total_packet_count < 1) last = high_resolution_clock::now();

				// dt
				auto now = high_resolution_clock::now();
				duration<double> dt_duration = now - last;
		        last = now;
		        double dt = dt_duration.count();
		        print_network("[server] dt: {}\n", dt);

				total_packet_count += 1;

				// print_network ("[server] packet received. total packet_count: {}\n", total_packet_count);
			 	Message_Type message_type = static_cast<Message_Type>(packet.header.message_type);
			 	std::string message_string = to_string(message_type);

				// who did we receive this from?
				bool player_found = false;
				for (auto& player_ip: server_connection_state.player_ips)
				{
					if (ipaddr == player_ip)
					{
						// print_network("[server] received packet from known player.\n");
						player_found = true;
					}
				}

				// if player is not found, but wants to connect:
				if (!player_found && packet.header.message_type == MESSAGE_JOIN_SERVER)
				{
					print_network("[server] player wants to join the server.\n");
					// i wanted to call this connect_player_to_server(player_ips, player_slots, ipaddr) but I only use it here so who cares
					// find the first empty slot

					for (int idx = 0; idx != sv_max_player_count; ++idx)
					{
						if (!server_connection_state.player_slots[idx])
						{
							server_connection_state.player_slots[idx] = true;
							server_connection_state.player_ips[idx] = ipaddr;
							Packet join_server_accepted_packet = construct_message_only_packet(MESSAGE_JOIN_SERVER_ACCEPTED);
							print_network("[server] connected player {} to server.\n", ipaddr);

							packet.header.timestamp = get_timestamp_microseconds();
							server_socket.send(join_server_accepted_packet, UDPsocket::IPv4::Broadcast(client_port_number));

							print_network("[server] sent acceptance packet.\n");
							break;
						}
					}
				}
				
				// non-owning, just point to one.
				Byte_Buffer* current_buffer_ptr = nullptr; 

				// if we have found the player, get the correct packet byte buffer (to reconstruct the full message if necessary.)
				if (player_found)
				{
					current_buffer_ptr = get_player_packet_byte_buffer_from_ip(server_connection_state, ipaddr);
				}

				if ((current_buffer_ptr == nullptr) && player_found) print_network("[server] current_buffer_ptr == nullptr but we found a player. that's not supposed to happen. crash incoming.\n");
				auto& buffer = current_buffer_ptr->data;
				auto& packet_header = packet.header;


				// dealing with player movement. everything below here is kind of stateful and tricky (setting things to default so we may override those defaults later.)
				auto move_input = Move_Input{}; // initialize to zero by default so we can simulate if there is no message sent.
				// what was this player's previous velocity?
				auto player_idx = get_player_idx(server_connection_state, ipaddr);
				Player_Movement_State& player_movement_state = get_player_movement_state(server_connection_state, player_idx);
				auto front = player_movement_state.front;
				auto right = cross(player_movement_state.front, vec3{0.f, 1.f, 0.f}); 

				// process all one-shot packages, ignore multi ones.
				if (packet_header.sequence_id == 0) 
				{
					// print_network("[server] one-shot package.\n");
					if (message_type == MESSAGE_PLAYER_MOVE)
					{
						// print_network("[server] received player move.\n");
						
						// extract the player movement out of the packet
						// is the message as large as the payload size?
						assert(sizeof(Player_Move_Message) == packet.header.payload_size);
						Player_Move_Message* player_move_message_ptr = reinterpret_cast<Player_Move_Message*>(packet.buffer);
						move_input = player_move_message_ptr->move_input;
						front = player_move_message_ptr->front;
						right = player_move_message_ptr->right;
					}

					// handle a player disconnecting gracefully.
					if (player_found && packet_header.message_type == Message_Type::MESSAGE_LEAVE_SERVER)
					{
						print_network("[server] disconnecting player {} from server.\n", ipaddr);
						disconnect_player(server_connection_state, ipaddr);
						total_packet_count = 0; // also reset this because otherwise dt fucks up.
					}
				}

				// simulate
				{
					auto player_aabb = AABB{.min = player_movement_state.position + player_collider_aabb.min, .max = player_movement_state.position + player_collider_aabb.max};
        			auto [collider_planes, all_face_indices] = collect_and_classify_intersecting_planes(map_bsp, map_vertices, player_aabb);
        			for (auto& plane: collider_planes.ground_planes)
        			{
        				std::print("ground plane: {}", plane);
        			}

	                auto [new_position, new_velocity] = player_move(
		                move_input,
		                collider_planes,
		                player_movement_state.position,
		                player_movement_state.velocity,
		                front, 
		                right,
		                dt);

					player_movement_state.position = new_position;
					player_movement_state.velocity = new_velocity;
					player_movement_state.front = front;
					print_network("[server] player_position: {}\n", new_position);
					
					// send a position back?
                	auto packets_to_send = convert_to_packets(Player_Move_Message{
                    .move_input = Move_Input{},
                    .front = front,
                    .right = player_movement_state.position}, Message_Type::MESSAGE_PLAYER_MOVE);
 					if (server_socket.send(packets_to_send[0], UDPsocket::IPv4::Broadcast(client_port_number)) < 0)
                    {
                    	print_warning("failed to send message.\n");
                    }
                    else 
                    {
                    	print_network("sent player move message.\n");
                    }

				}

				std::this_thread::sleep_for(milliseconds(16)); // ~60 FPS
			}
		}

		server_socket.close();
	});


	t1.join();

}

				// are we part of a sequence? // 

	// if (packet_header.sequence_id > 0)
	// 			{
	// 				print_warning("[server] sequenced packet stitching is not correctly implemented!\n");

	// 				print_network("[server] total packets in sequence: {}\n", packet_header.sequence_count);
	// 				assert(packet_header.sequence_count >= packet_header.sequence_idx);

	// 				std::memcpy(&buffer[byte_offset_from_start], &packet.buffer, packet_header.payload_size);
	// 				//@FIXME: actually, we should be really careful. this just inserts packages one behind the other.
	// 				// we need to reconstruct the actual payload.
	// 				byte_offset_from_start += packet_header.payload_size; 

	// 				// if this packet as part of a sequence, and the sequence is complete, hand it off to someone else.
	// 				if (packet_header.sequence_id > 0 && packet_count == packet_header.sequence_count)
	// 				{
	// 					// copy the buffer and zero the buffer.
	// 					auto handoff_buffer = std::vector<uint8_t>(byte_offset_from_start);
	// 					std::memcpy(&handoff_buffer[0], &buffer[0], byte_offset_from_start);
	// 					for(auto idx = 0; idx != byte_offset_from_start; ++idx)
	// 					{
	// 						buffer[idx] = uint8_t{0};
	// 					}

	// 					// process message (handover to different thread?)
	// 					// process_message(packet, std::move(handoff_buffer));
	// 				}
	// 			}
