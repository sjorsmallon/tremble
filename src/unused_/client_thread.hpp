

	// auto t2 = std::thread([&](){

	// 	UDPsocket client_socket{};
	// 	client_socket.open();
	// 	client_socket.bind(client_port_number);
	// 	client_socket.broadcast(true);
	// 						std::this_thread::sleep_for(std::chrono::seconds(1));
	// 	print_network("[client] started. listening on port {}\n", client_port_number);

	// 	std::this_thread::sleep_for(std::chrono::seconds(3));
	// 	print_network("[client] done sleeping.\n");

	// 	Packet packet = construct_message_only_packet(MESSAGE_JOIN_SERVER);
	// 	packet.header.timestamp = get_timestamp_microseconds();
	// 	client_socket.send(packet, UDPsocket::IPv4::Broadcast(server_port_number));
	// 	print_network("[client] sent join_server message.\n");
		
	// 	// wait for a response.
	// 	UDPsocket::IPv4 ipaddr{};
	// 	Packet incoming_packet{};
	// 	if (client_socket.recv(incoming_packet, ipaddr) < 0) // blocking
	// 	{
	// 		fprintf(stderr, "recv(): failed\n");
	// 	}
	// 	else
	// 	{
	// 		print_network("[client] received a packet.\n");
	// 		auto& header = packet.header;
	// 		if (header.message_type == MESSAGE_JOIN_SERVER_ACCEPTED)
	// 		{
	// 			print_network("[client] joined server.");
	// 		}

	// 		vec3 player_position = vec3{0.0f, 10.0f, 0.0f};
	// 		vec3 player_velocity = vec3{0.0f, 0.0f ,0.0f};
	// 		vec3 front{1.0f,0.0f, 0.0f};
	// 		vec3 up{0.0f, 1.0f, 0.0f};

	// 		float dt = 0 ;
	// 		while (true)
	// 		{
	// 			auto start_time = std::chrono::high_resolution_clock::now();

	// 			Move_Input move_input = generate_random_input();

	// 			// send input to the server.
	// 			std::vector<Packet> packets = convert_to_packets(move_input, MESSAGE_PLAYER_INPUT);
	// 			assert(packets.size() == 1);	
	// 			AABB_Traces traces{};
	// 			auto collider_planes = Collider_Planes{};
	// 			// simulate
	// 			auto [new_player_position, new_player_velocity] = my_walk_move(
	// 				move_input,
	// 				traces,
	// 				collider_planes,
	// 				player_position,
	// 				player_velocity,
	// 				front,
	// 				cross(front, up),
	// 				dt
	// 				);

	// 			player_position = new_player_position;
	// 			player_velocity = new_player_velocity;


	// 			std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// 			// Calculate time delta (dt) between frames
	// 	        auto end_time = std::chrono::high_resolution_clock::now();
	// 	        std::chrono::duration<float> elapsed_time = end_time - start_time;
	// 	        dt = elapsed_time.count();  // dt in seconds
	// 		}
	// 	}
	// });

	// t2.join();