#define NOMINMAX // fuck windows, all my homies hate windows
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_HANDLED
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>


#include <unordered_set>
#include <algorithm> // std::min...
#include <print>
#include <array>
#include <thread>

#include "../src/AABB.hpp"
#include "../src/bsp.hpp"
#include "../src/camera.hpp"
#include "../src/commands.hpp"
#include "../src/console.hpp"
#include "../src/debug_draw.hpp"
#include "../src/font.hpp"
#include "../src/gl_helpers.hpp"
#include "../src/input.hpp"
#include "../src/player_move.hpp"
#include "../src/udp_socket.hpp"
#include "../src/keys.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool g_noclip = false;
void toggle_noclip()
{
    g_noclip = !g_noclip;
}

// module local declaration
static Keys::Keycode sdl_convert_keyboard_key_to_our_own_key(SDL_Keycode sdl_key);
static void sdl_print_global_gl_attributes();
static void sdl_gl_set_attributes_before_creating_a_window();

// argc and argv[] are necessary for SDL3 main compatibility trickery.
int main(int argc, char *argv[])
{
    int window_width = 1920;
    int window_height = 1080;
	SDL_Window* window = nullptr;

    SDL_GLContext gl_context{}; // here because it needs to be destroyed.
    {
        SDL_SetAppMetadata("tremble", "1.0", "com.exa`mple.tremble");
        SDL_Init(SDL_INIT_VIDEO);
       
        sdl_gl_set_attributes_before_creating_a_window();
        // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
        window = SDL_CreateWindow(
            "Tremble", window_width, window_height, 
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        if (window == nullptr)
        {
            std::print("window is null. SDL_Error: {}\n", SDL_GetError());
            exit(1);
        }

        // Enable relative mouse mode and capture the mouse. move the mouse to the center of the window.
        // note that this does only do anything after the window has been created. (hidecursor. captuermouse, and warpinwindow.)
        {
            SDL_CaptureMouse(true);
            SDL_HideCursor();
            SDL_WarpMouseInWindow(window, 0.5f * window_width, 0.5f * window_height); // xy 
        }

        // Create an OpenGL context associated with the window.
        gl_context = SDL_GL_CreateContext(window);

        // for some own context.
        sdl_print_global_gl_attributes();

        // do some trickery with glad to actually load modern opengl.
        gladLoadGL();

        // huh. even though I did not do this, it seems to not matter.
        SDL_GL_MakeCurrent(window, gl_context);

        // enable vsync
        SDL_GL_SetSwapInterval(1);

        glh_set_global_gl_settings();
    }    

    // font stuff
    auto font_size = 22.f;
    // Font font = create_font_at_size("../data/fonts/CONSOLA.ttf", font_size);
    Font font = create_font_at_size("../data/fonts/lucon.ttf", font_size);
    const int font_atlas_width = 512;
    const int font_atlas_height = 512;
    Font_Texture_Atlas font_texture_atlas = create_font_texture_atlas(font, font_atlas_width, font_atlas_height);

    // create a gl texture.
    GL_Texture font_bitmap_texture = register_texture_opengl(
        font_texture_atlas.atlas_bitmap,
        font_texture_atlas.width,
        font_texture_atlas.height,
        Texture_Format::Red // put it in the red part of the buffer.
        );

    // load the orange 128 * 128 texture.
    const char* texture_name = "../data/textures/orange_with_text.png";
    auto texture_width = int32_t{}; 
    auto texture_height = int32_t{};
    auto channels_in_file = int32_t{};
    auto desired_channels = int32_t{4};

    // we own this? we need to free this?
    uint8_t* data = stbi_load(texture_name, &texture_width, &texture_height, &channels_in_file, desired_channels);
    std::print("x: {}, texture_height: {}, channels_in_file: {}\n", texture_width, texture_height, channels_in_file);
    std::vector<uint8_t> unfolded_data(data, data + ((texture_width * texture_height * channels_in_file) / sizeof(uint8_t))); // what the fuck is this?

    auto wall_texture = register_texture_opengl(unfolded_data, texture_width, texture_height, Texture_Format::RGBA);
    auto wall_width = int32_t{1024};
    auto wall_height = int32_t{1024};
    auto wall_vertices = generate_vertex_xu_quad_from_plane(vec3{0.f, 0.f, 0.f}, vec3{0.f, 0.f, -1.f}, wall_width, wall_height);
    float tile_size = 256;
    auto wall_tile_scale = float{1024 / tile_size};
    std::print("wall_tile_scale: {}\n", wall_tile_scale);
    // note: this kind of sucks to deal with but I don't care at the moment. all uv shit is flipped because
    // I don't like drawing things upside down and the wrong way around.
    for(auto& vertex: wall_vertices)
    {
        vertex.uv.u = 1.f - vertex.uv.u;
        vertex.uv.v = 1.f - vertex.uv.v;
    }
    auto wall_gl_buffer = create_interleaved_xu_buffer(wall_vertices);

    // set up the text buffer
    //@FIXME: there have to be better ways to do this. we should move to index buffers.
    constexpr auto max_character_count_in_string = size_t{512};
    constexpr auto vertices_per_character = size_t{6};

    // since when do I care about this stuff actually?
    auto text_character_vertices = std::vector<vertex_xu>{max_character_count_in_string * vertices_per_character};
    auto text_characters_gl_buffer = create_interleaved_xu_buffer(text_character_vertices);

    // shaders
    uint32_t xnc_shader_program = create_interleaved_xnc_shader_program();

    auto xu_vertex_shader_string = file_to_string("../data/shaders/vertex_xu/vertex_xu.vert");
    auto xu_fragment_shader_string = file_to_string("../data/shaders/vertex_xu/vertex_xu.frag");
    uint32_t xu_shader_program = create_shader_program(xu_vertex_shader_string.c_str(), xu_fragment_shader_string.c_str());
    set_uniform(xu_shader_program,"base_texture", 1);

    auto text_vertex_shader_string = file_to_string("../data/shaders/text/text.vert");
    auto text_fragment_shader_string = file_to_string("../data/shaders/text/text.frag");
    uint32_t text_shader_program = create_shader_program(text_vertex_shader_string.c_str(), text_fragment_shader_string.c_str());
    set_uniform(text_shader_program, "text_bitmap", 0);

    auto x_vertex_shader_string = file_to_string("../data/shaders/vertex_color_x/vertex_color_x.vert");
    auto x_fragment_shader_string = file_to_string("../data/shaders/vertex_color_x/vertex_color_x.frag");
    uint32_t x_shader_program = create_shader_program(x_vertex_shader_string.c_str(), x_fragment_shader_string.c_str());

    auto tiled_vertex_xu_vertex_shader_string = file_to_string("../data/shaders/tiled_vertex_xu/tiled_vertex_xu.vert");
    auto tiled_vertex_xu_fragment_shader_string = file_to_string("../data/shaders/tiled_vertex_xu/tiled_vertex_xu.frag");
    uint32_t tiled_vertex_xu_shader_program = create_shader_program(tiled_vertex_xu_vertex_shader_string.c_str(), tiled_vertex_xu_fragment_shader_string.c_str());
    set_uniform(tiled_vertex_xu_shader_program, "base_texture", 1);
    set_uniform(tiled_vertex_xu_shader_program, "tile_scale", wall_tile_scale);


    // base geometry
    auto path = std::string{"../data/just_a_floor_AABBs"};
    auto aabbs = read_AABBs_from_file(path);
    auto world_map_vertices  = to_vertex_xnc(aabbs);
    // i do not trust auto assignment.
    std::vector<vertex_xnc> base_aabbs_vertices = world_map_vertices;
    auto aabb_gl_buffer = create_interleaved_xnc_buffer(world_map_vertices);
    BSP* world_map_bsp = build_bsp(world_map_vertices); //@FIXME: this is never freed. nice.

    // console geometry
    auto console_min = vec2{.x = 0.f, .y = 0.5f * static_cast<float>(window_height)}; // upper half of the screen.
    auto console_max = vec2{.x = static_cast<float>(window_width), .y = static_cast<float>(window_height)};
    auto console_background_vertices = generate_vertex_x_quad(console_min, console_max);
    auto text_entry_bar_min = vec2{.x = 0.f, .y = 0.5f * static_cast<float>(window_height) - 2 * font_size};
    auto text_entry_bar_max =vec2{.x = static_cast<float>(window_width), .y = text_entry_bar_min.y +  2 *font_size};
    auto text_entry_bar_vertices = generate_vertex_xu_quad(text_entry_bar_min, text_entry_bar_max);

    // I don't care.
    auto console_background_gl_buffer = create_x_buffer(console_background_vertices);
    auto text_entry_bar_gl_buffer = create_interleaved_xu_buffer(text_entry_bar_vertices);

    // game loop state
    bool running = true;
    SDL_Event event;
    float dt = 0.f;
    float mouse_x{};
    float mouse_y{};
    float last_mouse_x{};
    float last_mouse_y{};
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0.f;
   
    // game systems
    Console console{}; // in-game console
    Command_System command_system{};
    register_command(command_system, "noclip", toggle_noclip);

    // game state.
    // bool noclip = false; -> supplanted by g_noclip now.
    bool showing_console = false;

    auto camera = Camera{};
    camera.yaw   = -90.f;
    camera.pitch = 0.f;
    float noclip_move_speed = 1000.0f;
    float mouse_sensitivity = 1.f;
    float fov = 105.0f;
    float near_z = 0.1f;
    float far_z = 4000.f; // the projection matrix z should be as far as the bounds of the map. I think this is the way quake does it.
    vec3 world_up = vec3{0.f, 1.f, 0.f};

    // entity related
    vec3 player_velocity{};
    // vec3 player_position{-6.0320406, 10, 580.2726};
    vec3 player_position{0.0f, 10.f, 0.f};

    auto player_aabb = AABB{.min = vec3{-20.0f, -20.0f, -20.0f}, .max = {20.0f, 45.f, 20.f}}; // 40 x, 65 y (20 off the floor), 40 z. //@volatile: this needs to stay the same between server and client (so we should store it somewhere central.)
    auto player_aabb_vertices = to_vertex_xnc(player_aabb);
    auto player_aabb_gl_buffer = create_interleaved_xnc_buffer(player_aabb_vertices);

    auto server_player_aabb = AABB{};
    auto server_player_aabb_vertices = to_vertex_xnc(server_player_aabb);
    auto server_player_aabb_gl_buffer = create_interleaved_xnc_buffer(server_player_aabb_vertices);


    Move_Input move_input{};
     // so we can render the "last" colliding hitbox so I can do some manual inspection.
    glm::mat4 aabb_transform_matrix(1.0f);
    std::vector<size_t> previous_face_indices{};


    // server stuff
    bool playing_online = false;
    constexpr auto server_port_number = 2020;
    constexpr auto client_port_number = 2024;
    UDPsocket::IPv4 ipaddr{};

    UDPsocket client_socket{};
    {
        client_socket.open();
        client_socket.broadcast(true);
        client_socket.bind(client_port_number);

        auto join_server_request_packet = construct_message_only_packet(Message_Type::MESSAGE_JOIN_SERVER);

        // try to connect to the server.
        if (client_socket.send(join_server_request_packet, UDPsocket::IPv4::Broadcast(server_port_number)) < 0)
        {
            print_network("[client] send join_server_request failed.\n");
        }
        else 
        {
            print_network("[client] send message succeeded.\n");
        }
    }

    const int MAX_WAIT_TIME_MS = 100;
    const int CHECK_INTERVAL_MS = 10;
    auto join_server_result_packet = Packet{};
    auto join_result  = client_socket.recv_nonblocking_but_try_every_so_often(join_server_result_packet, ipaddr, MAX_WAIT_TIME_MS, CHECK_INTERVAL_MS);
  
    if (join_result > 0 && join_server_result_packet.header.message_type == Message_Type::MESSAGE_JOIN_SERVER_ACCEPTED)
    {
        print_network("[client] join server accepted!\n");
        playing_online = true;
    }
    else
    {
        print_network("[client] playing offline.\n");
    }


    size_t fps = 0;
    size_t frame_counter = 0;
    while (running)
    {
        last = now;
        //@NOTE: SDL_GetPerformanceCounter is too high precision for floats. If I make it double "it just works". (SDL_GetPeformanceCOunter is actually uint64_t).
        now = SDL_GetPerformanceCounter();
        dt = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) / 1000.0; // Convert to seconds

        fps = double{1.0} / dt;
        frame_counter += 1;


        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;

        // handle quitting etc. 
         while (SDL_PollEvent(&event))
        {
            // ignore all events except quit (alt-f4, pressing x, etc)
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_EVENT_KEY_UP) // not key down? should we add a grace period?
            {
                if ((event.key.key == SDLK_TILDE) || (event.key.key == SDLK_GRAVE))
                {   
                    showing_console = !showing_console;
                }
            }
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                if (showing_console)
                {   

                    bool shift_pressed = (SDL_GetModState() & SDL_KMOD_SHIFT);
                    bool control_pressed = (SDL_GetModState() & SDL_KMOD_CTRL);
                    uint32_t key = sdl_convert_keyboard_key_to_our_own_key(event.key.key);

                    handle_keystroke(console, event.key.key, shift_pressed, control_pressed); 

                    //@Note: for now, do it disjointed, so we do not tangle the systems at this point already. This is a good thing!
                    if (key == KEY_RETURN)
                    {
                        execute_command(command_system, console.history.back());
                        //@Hack: reset player velocity. should actually be part of the command.
                        player_velocity = vec3{0};
                    }
                }
            }

            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
            {
                if (event.button.button == SDL_BUTTON_X1)
                {
                    // noclip = !noclip;
                    toggle_noclip();
                }
            }
        }

        // handle keyboard input. this is polling, which seemed more reliable. although I do not remember why. 
        {
            const bool* key_state = SDL_GetKeyboardState(NULL);
            if (key_state[SDL_SCANCODE_ESCAPE]) running = false;

            move_input.forward_pressed  = key_state[SDL_SCANCODE_W];
            move_input.backward_pressed = key_state[SDL_SCANCODE_S];
            move_input.left_pressed     = key_state[SDL_SCANCODE_A];
            move_input.right_pressed    = key_state[SDL_SCANCODE_D];
            move_input.jump_pressed     = key_state[SDL_SCANCODE_SPACE];

            // P -> print player position info.
            if (key_state[SDL_SCANCODE_P])
            {
                    std::print("player_position: {}\n", player_position);
                    std::print("camera.front: {}\n", camera.front);
                    std::print("camera.pitch: {}\n", camera.pitch);
            }

            // collision detection (before moving the player, I guess. That's why it is in keyboard handling.)

            std::vector<size_t> face_indices;
            // restore color to the previous face_indices
            for (auto& face_idx: previous_face_indices)
            {
                std::array<vertex_xnc, 3> intersecting_face{
                    base_aabbs_vertices[face_idx],
                    base_aabbs_vertices[face_idx + 1],
                    base_aabbs_vertices[face_idx + 2]
                };
                auto face_offset = face_idx * sizeof(vertex_xnc);
            
                // color the triangle white.
                glBindVertexArray(aabb_gl_buffer.VAO); 
                glBindBuffer(GL_ARRAY_BUFFER, aabb_gl_buffer.VBO);

                GLsizeiptr size = 3 * sizeof(vertex_xnc); // replace vertex color for the entire face.
                glBufferSubData(GL_ARRAY_BUFFER, face_offset, size, (void*)intersecting_face.data());

                glBindVertexArray(0);
            }

            // update player_aabb to world space (to visualize any collisions.)
            auto aabb = AABB{.min = player_position + player_aabb.min, .max = player_position + player_aabb.max};
            auto [collider_planes, collider_face_indices] = collect_and_classify_intersecting_planes(world_map_bsp, world_map_vertices, aabb);

            if (g_noclip)
            {
                face_indices = previous_face_indices;
            }
            else
            {
                face_indices = collider_face_indices;
            }


            // color the intersecting face indices white.
            for (auto& face_idx: face_indices)
            {
                std::array<vertex_xnc, 3> intersecting_face{
                    world_map_vertices[face_idx],
                    world_map_vertices[face_idx + 1],
                    world_map_vertices[face_idx + 2]
                };
            
                intersecting_face[0].color = vec4{1.0f,1.0f,1.0f,1.0f};
                intersecting_face[1].color = vec4{1.0f,1.0f,1.0f,1.0f};
                intersecting_face[2].color = vec4{1.0f,1.0f,1.0f,1.0f};
                
                // color the triangle white.
                glBindVertexArray(aabb_gl_buffer.VAO); 
                glBindBuffer(GL_ARRAY_BUFFER, aabb_gl_buffer.VBO);
                auto face_offset = face_idx * sizeof(vertex_xnc);

                GLsizeiptr size = 3 * sizeof(vertex_xnc); // replace vertex color for the entire face.
                glBufferSubData(GL_ARRAY_BUFFER, face_offset, size, (void*)intersecting_face.data());

                glBindVertexArray(0);
            }

            previous_face_indices = face_indices;

            if (!g_noclip)
            {
                // first, update the player position and velocity.
                glm::vec3 right = glm::cross(camera.front, camera.up);

                auto [new_position, new_velocity] = player_move(
                    move_input,
                    collider_planes,
                    player_position,
                    player_velocity,
                    vec3{camera.front.x, camera.front.y, camera.front.z},
                    vec3{right.x, right.y, right.z},
                    dt);

                // send the move input to the server.
                auto packets_to_send  = convert_to_packets(Player_Move_Message{
                    .move_input = move_input,
                    .front = vec3{camera.front.x, camera.front.y, camera.front.z},
                    .right = vec3{right.x, right.y, right.z}}, Message_Type::MESSAGE_PLAYER_MOVE);

                assert(packets_to_send.size() == 1);

                // try to send an update for 5 ms.
                if (playing_online)
                {
                     auto result = execute_with_timeout([&]() -> bool
                    {
                        if (client_socket.send(packets_to_send[0], UDPsocket::IPv4::Broadcast(server_port_number)) < 0)
                        {
                        }
                        else 
                        {
                            return true;
                        }

                        return false;
                    }, 10);

                    if (result)
                    {
                        // print_network("[client] sent input.\n");
                    }
                    else
                    {
                        print_warning("could not send message in 10 ms time allotted. continuing..\n");
                    }
                }

                player_position = new_position;
                player_velocity = new_velocity;

                camera.position = glm::vec3(new_position.x, new_position.y, new_position.z);
            }
            else // noclip
            {
                camera = update_camera(camera, dt,
                    move_input.forward_pressed,
                    move_input.left_pressed,
                    move_input.backward_pressed,
                    move_input.right_pressed,
                    noclip_move_speed);
                player_position = vec3{camera.position.x, camera.position.y, camera.position.z};
            }
        }

        // handle mouse input.
        {
            uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
            if (!mouse_state) {} //@Note this is just to stop the whining about mouse_state being unused.
            int dx = mouse_x - last_mouse_x;
            int dy = mouse_y - last_mouse_y;
            camera = look_around(camera, dx, dy, mouse_sensitivity);

        }

        // rendering code goes here.
        {
            glClearColor(7.f/255.f, 7.f/255.f, 7.f/255.f, 1); // nsight says this is not allowed?
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

            // aabbs.
            draw_triangles(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, xnc_shader_program,
                glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                get_look_at_view_matrix(camera),
                glm::mat4(1.0f),
                false // wireframe
                );

            // upon entering noclip, draw the last collision.
            if (g_noclip)
            {
                //  also draw the aabb at the last position.
                draw_triangles(player_aabb_gl_buffer.VAO, player_aabb_gl_buffer.VBO, player_aabb_gl_buffer.vertex_count, xnc_shader_program,
                glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                get_look_at_view_matrix(camera),
                aabb_transform_matrix,
                false //wireframe
                );

                // draw the faces we collided with (I was using this to disable drawing the rest, and just the colliding faces for debugging.)
                {
                    auto construct_faces = [](const std::vector<vertex_xnc>& world_map_vertices, const std::vector<size_t>& face_indices) {
                        std::vector<vertex_xnc> faces;
                        for (size_t face_idx = 0; face_idx < face_indices.size(); ++face_idx)
                        {
                                auto& v0 = world_map_vertices[face_indices[face_idx]];
                                auto& v1 = world_map_vertices[face_indices[face_idx] + 1];
                                auto& v2 = world_map_vertices[face_indices[face_idx] + 2];
                                faces.push_back(v0);   
                                faces.push_back(v1);
                                faces.push_back(v2);
                        }

                        return faces;
                    };

                    auto faces = construct_faces(world_map_vertices, previous_face_indices);
                    if (faces.size() > 0)
                    {
                        debug_draw_triangles(
                            faces, 
                        glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                        get_look_at_view_matrix(camera),
                        glm::mat4(1.0f));
                    }
                }

            }
        
            // draw the textured wall.
            draw_triangles(
                wall_gl_buffer.VAO,
                wall_gl_buffer.VBO,
                wall_gl_buffer.vertex_count, tiled_vertex_xu_shader_program,
                glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                get_look_at_view_matrix(camera),
                glm::mat4(1.0f),
                false // wireframe
            );


            //see if the server has sent anything back.
            if (playing_online)
            {
                Packet packet{};

                if (client_socket.recv(packet, ipaddr) < 0) // blocking
                {
                    print_network("[client] recv failed while waiting for a position.\n");
                }
                else
                {
                    // return true;
                }
           

                if (true)
                {
                    if (packet.header.message_type == MESSAGE_PLAYER_MOVE)
                    {
                        print_network("received player move data.\n");
                        // we have the correct data.
                        Player_Move_Message* player_move_message_ptr = reinterpret_cast<Player_Move_Message*>(packet.buffer);
                        std::print("network player position: {}\n", player_move_message_ptr->right);
                        // abuse the right value by interpreting it as the position.
                        auto position = player_move_message_ptr->right;
                        server_player_aabb = AABB{.min = position + player_aabb.min, .max = position + player_aabb.max};
                        server_player_aabb_vertices = to_vertex_xnc(server_player_aabb);

                        // fully replace the buffer because why not.
                        glBindVertexArray(server_player_aabb_gl_buffer.VAO); 
                        glBindBuffer(GL_ARRAY_BUFFER, server_player_aabb_gl_buffer.VBO);
                        glBufferData(GL_ARRAY_BUFFER, server_player_aabb_vertices.size() * sizeof(vertex_xnc), server_player_aabb_vertices.data(), GL_STATIC_DRAW);

                        draw_triangles(server_player_aabb_gl_buffer.VAO, server_player_aabb_gl_buffer.VBO, server_player_aabb_gl_buffer.vertex_count, xnc_shader_program,
                        glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                        get_look_at_view_matrix(camera),
                        aabb_transform_matrix,
                        true //wireframe
                        );

                    }
                }

            }


            if (showing_console)
            {
                // map everything between -1 z and + 1 z. so all of these vertices should have a z between those bounds.
                float min_z = -1.0f;
                float max_z = 1.0f;
                glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(window_width), 0.0f, static_cast<float>(window_height), min_z, max_z);
                // draw background
                set_uniform(x_shader_program, "color", vec4{7.0f/255.f, 38.0f/255.f, 38.0f/255.f, .5f});

                // console background
                draw_triangles(
                    console_background_gl_buffer.VAO,
                    console_background_gl_buffer.VBO,
                    console_background_gl_buffer.vertex_count,
                    x_shader_program,
                    projection,
                    glm::mat4(1.0f), // identity view matrix.
                    glm::mat4(1.0f), // identity transformation matrix.
                    false
                    );

                set_uniform(x_shader_program, "color", vec4{27.0f/255.f, 27.0f/255.f, 27.0f/255.f, .8f});
                // text entry bar.
                draw_triangles(
                    text_entry_bar_gl_buffer.VAO,
                    text_entry_bar_gl_buffer.VBO,
                    text_entry_bar_gl_buffer.vertex_count,
                    x_shader_program,
                    projection,
                    glm::mat4(1.0f), // identity view matrix.
                    glm::mat4(1.0f), // identity transformation matrix.
                    false
                    );


                    auto draw_text_line = [](
                        std::string_view line,
                        Font_Texture_Atlas& atlas,
                        int start_x,
                        int start_y,
                        GL_Buffer text_buffer,
                        uint32_t shader_program,
                        glm::mat4& projection_matrix
                        )
                    {
                        auto max_characters = text_buffer.vertex_count / 6;
                        assert(line.size() < max_characters);
                        
                        float x_offset = start_x;
                        float y_offset = start_y;
                       
                        // make the vbo active
                        glBindBuffer(GL_ARRAY_BUFFER, text_buffer.VBO);
                        //FIXME: this is not nice. but I do not want to fumble with a new data type now. this just counts until we hit the first '\0'.
                        auto characters_to_render = std::strlen(line.data());

                        for (int idx = 0; idx != characters_to_render; ++idx)
                        {
                            char character = line[idx];

                            int ascii_idx = character - 32; // magic ascii value described in font.hpp (we map the ascii range [32, .. , 32 +96])
                            if (ascii_idx < 0) std::print("[error] character ascii_idx < 0. character: {}\n", character);

                            // get bounding box of this character?
                            stbtt_packedchar& character_info = atlas.character_info[ascii_idx];
                            stbtt_aligned_quad quad{}; 

                            // float x0,y0,s0,t0; // top-left
                            // float x1,y1,s1,t1; // bottom-right
                            stbtt_GetPackedQuad(
                                atlas.character_info,
                                atlas.width,
                                atlas.height,  // same data as above
                                static_cast<int>(character) - 32,             // character to display
                               &x_offset, &y_offset,   // pointers to current position in screen pixel space (these are advanced in this function.)
                               &quad,      // output: quad to draw
                               0);

                            // update the uv mappings.
                            // top left, bottom left, bottom right.
                            // top left, bottom right, top right.
                            std::array<vertex_xu, 6> text_character_vertices{};
                            auto& v0 = text_character_vertices[0];
                            auto& v1 = text_character_vertices[1];
                            auto& v2 = text_character_vertices[2];
                            auto& v3 = text_character_vertices[3];
                            auto& v4 = text_character_vertices[4];
                            auto& v5 = text_character_vertices[5];

                            float character_width = fabs(static_cast<float>(character_info.x1) - static_cast<float>(character_info.x0));
                            // float character_width = character_info.xadvance;
                            
                            bool flip = true;
                            if (flip)
                            {
                                  // // swap them?
                                if (quad.y0 > quad.y1) std::swap(quad.y0, quad.y1);

                                // what's the delta between y0 and the y line?
                                float y0_delta = fabs(quad.y0 - y_offset);

                                // what's the delta between y1 and the y line?
                                float y1_delta = fabs(quad.y1 - y_offset);

                                if (quad.y1 > y_offset && quad.y0 < y_offset)
                                {
                                    quad.y1 = quad.y1 + fabs(y0_delta - y1_delta);
                                    quad.y0 = quad.y0 + fabs(y0_delta - y1_delta);
                                }
                                else
                                {
                                   // add y0 delta to both y0 and y1
                                    quad.y0 += y0_delta;
                                    quad.y1 += y0_delta;
                                }

                                // top left, bot left, bot right
                                // top left,  bot right, top right
                                // since text is on top, give it a z value 0.1f (+z is closer to the camera)
                                v0 = vertex_xu{.position = vec3{quad.x0, quad.y1, 0.1f}, .uv = vec2{quad.s0, quad.t0}};
                                v1 = vertex_xu{.position = vec3{quad.x0, quad.y0, 0.1f}, .uv = vec2{quad.s0, quad.t1}};
                                v2 = vertex_xu{.position = vec3{quad.x1, quad.y0, 0.1f}, .uv = vec2{quad.s1, quad.t1}};
                                v3 = vertex_xu{.position = vec3{quad.x0, quad.y1, 0.1f}, .uv = vec2{quad.s0, quad.t0}};
                                v4 = vertex_xu{.position = vec3{quad.x1, quad.y0, 0.1f}, .uv = vec2{quad.s1, quad.t1}};
                                v5 = vertex_xu{.position = vec3{quad.x1, quad.y1, 0.1f}, .uv = vec2{quad.s1, quad.t0}};

                            }
                            else
                            {
                                vec2 top_left{quad.x0, quad.y0};
                                vec2 bot_right{quad.x1, quad.y1};
                                vec2 bot_left{quad.x0, quad.y1};
                                vec2 top_right{quad.x1, quad.y0};

                                vec2 top_left_uv{quad.s0, quad.t0};
                                vec2 bot_right_uv{quad.s1, quad.t1};
                                vec2 bot_left_uv{quad.s0, quad.t1};
                                vec2 top_right_uv{quad.s1, quad.t0};
                              
                                v0 = vertex_xu{.position = vec3{top_left.x, top_left.y, 0.1f}, .uv = top_left_uv};
                                v1 = vertex_xu{.position = vec3{bot_left.x, bot_left.y, 0.1f}, .uv = bot_left_uv};
                                v2 = vertex_xu{.position = vec3{bot_right.x, bot_right.y, 0.1f}, .uv = bot_right_uv};
                                v3 = vertex_xu{.position = vec3{top_left.x, top_left.y, 0.1f}, .uv = top_left_uv};
                                v4 = vertex_xu{.position = vec3{bot_right.x, bot_right.y, 0.1f}, .uv = bot_right_uv};
                                v5 = vertex_xu{.position = vec3{top_right.x, top_right.y, 0.1f}, .uv = top_right_uv};
                            }

                            // where in the buffer are we ?
                            int offset = idx * ( 6 * sizeof(vertex_xu));
                            //@note: we should actually map the buffer instead of doing this. but it's whatever.
                            glBufferSubData(GL_ARRAY_BUFFER, offset, text_character_vertices.size() * sizeof(vertex_xu), text_character_vertices.data());
                        }

                        draw_triangles(
                            text_buffer.VAO,
                            text_buffer.VBO,
                            line.size() * 6, // vertex count 
                            shader_program,
                            projection_matrix,
                            glm::mat4(1.0f), // identity view matrix.
                            glm::mat4(1.0f), // identity transform,
                            false);

                    };

                    // requires font_texture_atlas -> font, console.
                    // draw the console. write it out, and go from there.
                    const auto line_height = font.line_height;
                    constexpr auto spacing = 5; // 5 pixels spacing between line
                    auto start_x = 10; 
                    auto console_height = fabs(console_max.y - console_min.y);
                    // render as many history lines as we can fit.
                    const size_t max_lines_to_render = console_height / (line_height + spacing); // character_height?

                    int start_y = console_min.y;
                    glm::mat4 ortographic_projection_matrix = glm::ortho(0.0f, static_cast<float>(window_width), 0.0f, static_cast<float>(window_height),  min_z, max_z);
                    //Render the history lines (this seems hilariously bad.)
                    auto history_size = console.history.size(); // this is not constant: this is the occupied count.
                    auto latest_entry_idx = console.history.index_of_latest_entry();

                    // Determine the number of lines to render (either `history_size` or `max_lines_to_render`, whichever is smaller)

                    int number_of_lines_to_render = std::min(history_size, max_lines_to_render);
                    for (int line_idx = 0; line_idx != number_of_lines_to_render; ++line_idx)
                    {
                        size_t render_idx = console.history.wrap_index(latest_entry_idx - line_idx);

                        // what is the newest index? render bottom to top.
                        // that's back, right?
                        std::string line;
                        if (console.history.get(render_idx, line))
                        {
                            int current_y = start_y + line_idx * (line_height + spacing);
                            // Calculate the current y position for rendering
                             draw_text_line(
                            std::string_view{line},
                            font_texture_atlas,
                            start_x,
                            current_y,
                            text_characters_gl_buffer,
                            text_shader_program, ortographic_projection_matrix);
                        }
                    }

                    int text_entry_y = text_entry_bar_min.y + (abs(text_entry_bar_min.y -  text_entry_bar_max.y) /  2);
                    // draw the input text
                    draw_text_line(
                        std::string_view{console.input_buffer},
                        font_texture_atlas,
                        start_x,
                        text_entry_y,
                        text_characters_gl_buffer,
                        text_shader_program, ortographic_projection_matrix);

            }





        }

        SDL_GL_SwapWindow(window);
    }


    // send a quick disconnect message (for my own sake.)
    Packet leave_server_packet = construct_message_only_packet(Message_Type::MESSAGE_LEAVE_SERVER);

    // this is not coming through. why not?
    if (playing_online)
    {
        if (client_socket.send(leave_server_packet, UDPsocket::IPv4::Broadcast(server_port_number)) < 0)
        {
            print_network("[client] SENDING FAILED!\n");
        }
        else
        {
            print_network("[client] disconnecting.\n");
        }    
    }

    SDL_GL_DestroyContext(gl_context);

    return 0;
}






static void sdl_gl_set_attributes_before_creating_a_window()
{
    constexpr const uint32_t r_opengl_major_version = 4;
    constexpr const uint32_t r_opengl_minor_version = 5;
    constexpr const uint32_t r_single_color_channel_bit_width = 16;
    constexpr const uint32_t r_depth_buffer_bit_width = 24;
    constexpr const bool r_enable_double_buffering = true;
    constexpr const uint32_t r_stencil_buffer_bit_width = 8;
    // set gl attributes        
    {
        bool result;
        result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, r_opengl_major_version);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4): {}\n", result);
        result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, r_opengl_minor_version);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5): {}\n", result);
        result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE): {}\n", result);
        result = SDL_GL_SetAttribute(SDL_GL_RED_SIZE, r_single_color_channel_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_RED_SIZE,16): {}\n", result);      // 16 bits for red channel
        result = SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,r_single_color_channel_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,16): {}\n", result);    // 16 bits for green channel
        result = SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,r_single_color_channel_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,16): {}\n", result);     // 16 bits for blue channel
        result = SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,r_single_color_channel_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,16): {}\n", result);    // 16 bits for alpha channel
        result = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depth_buffer_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24): {}\n", result);   // 24 bits for depth buffer
        result = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, r_enable_double_buffering);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1): {}\n", result);  // Enable double buffering
        result = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, r_stencil_buffer_bit_width);
        if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
        std::print("SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8): {}\n", result);  // 8 bits for stencil buffer
    }
}


static void sdl_print_global_gl_attributes()
{
        // get gl attributes
    {
        bool result;
        int value;
        result = SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_RED_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_RED_SIZE: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,&value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,&value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE,&value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER: {}\n", value);
        result = SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value);
        if (!result) std::print("SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE failed.\n");
        std::print("SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE: {}\n", value);
    }
}


static Keys::Keycode sdl_convert_keyboard_key_to_our_own_key(SDL_Keycode sdl_key)
{

static auto sdl_key_mapping  = std::unordered_map<SDL_Keycode, Keys::Keycode>{
   {SDLK_UNKNOWN, KEY_UNKNOWN},
   {SDLK_RETURN, KEY_RETURN},
   {SDLK_ESCAPE, KEY_ESCAPE},
   {SDLK_BACKSPACE, KEY_BACKSPACE},
   {SDLK_TAB, KEY_TAB},
   {SDLK_SPACE, KEY_SPACE},
   {SDLK_EXCLAIM, KEY_EXCLAIM},
   {SDLK_DBLAPOSTROPHE, KEY_DBLAPOSTROPHE},
   {SDLK_HASH, KEY_HASH},
   {SDLK_DOLLAR, KEY_DOLLAR},
   {SDLK_PERCENT, KEY_PERCENT},
   {SDLK_AMPERSAND, KEY_AMPERSAND},
   {SDLK_APOSTROPHE, KEY_APOSTROPHE},
   {SDLK_LEFTPAREN, KEY_LEFTPAREN},
   {SDLK_RIGHTPAREN, KEY_RIGHTPAREN},
   {SDLK_ASTERISK, KEY_ASTERISK},
   {SDLK_PLUS, KEY_PLUS},
   {SDLK_COMMA, KEY_COMMA},
   {SDLK_MINUS, KEY_MINUS},
   {SDLK_PERIOD, KEY_PERIOD},
   {SDLK_SLASH, KEY_SLASH},
   {SDLK_0, KEY_0},
   {SDLK_1, KEY_1},
   {SDLK_2, KEY_2},
   {SDLK_3, KEY_3},
   {SDLK_4, KEY_4},
   {SDLK_5, KEY_5},
   {SDLK_6, KEY_6},
   {SDLK_7, KEY_7},
   {SDLK_8, KEY_8},
   {SDLK_9, KEY_9},
   {SDLK_COLON, KEY_COLON},
   {SDLK_SEMICOLON, KEY_SEMICOLON},
   {SDLK_LESS, KEY_LESS},
   {SDLK_EQUALS, KEY_EQUALS},
   {SDLK_GREATER, KEY_GREATER},
   {SDLK_QUESTION, KEY_QUESTION},
   {SDLK_AT, KEY_AT},
   {SDLK_LEFTBRACKET, KEY_LEFTBRACKET},
   {SDLK_BACKSLASH, KEY_BACKSLASH},
   {SDLK_RIGHTBRACKET, KEY_RIGHTBRACKET},
   {SDLK_CARET, KEY_CARET},
   {SDLK_UNDERSCORE, KEY_UNDERSCORE},
   {SDLK_GRAVE, KEY_GRAVE},
   {SDLK_A, KEY_A},
   {SDLK_B, KEY_B},
   {SDLK_C, KEY_C},
   {SDLK_D, KEY_D},
   {SDLK_E, KEY_E},
   {SDLK_F, KEY_F},
   {SDLK_G, KEY_G},
   {SDLK_H, KEY_H},
   {SDLK_I, KEY_I},
   {SDLK_J, KEY_J},
   {SDLK_K, KEY_K},
   {SDLK_L, KEY_L},
   {SDLK_M, KEY_M},
   {SDLK_N, KEY_N},
   {SDLK_O, KEY_O},
   {SDLK_P, KEY_P},
   {SDLK_Q, KEY_Q},
   {SDLK_R, KEY_R},
   {SDLK_S, KEY_S},
   {SDLK_T, KEY_T},
   {SDLK_U, KEY_U},
   {SDLK_V, KEY_V},
   {SDLK_W, KEY_W},
   {SDLK_X, KEY_X},
   {SDLK_Y, KEY_Y},
   {SDLK_Z, KEY_Z},
   {SDLK_LEFTBRACE, KEY_LEFTBRACE},
   {SDLK_PIPE, KEY_PIPE},
   {SDLK_RIGHTBRACE, KEY_RIGHTBRACE},
   {SDLK_TILDE, KEY_TILDE},
   {SDLK_DELETE, KEY_DELETE},
   {SDLK_PLUSMINUS, KEY_PLUSMINUS},
   {SDLK_CAPSLOCK, KEY_CAPSLOCK},
   {SDLK_F1, KEY_F1},
   {SDLK_F2, KEY_F2},
   {SDLK_F3, KEY_F3},
   {SDLK_F4, KEY_F4},
   {SDLK_F5, KEY_F5},
   {SDLK_F6, KEY_F6},
   {SDLK_F7, KEY_F7},
   {SDLK_F8, KEY_F8},
   {SDLK_F9, KEY_F9},
   {SDLK_F10, KEY_F10},
   {SDLK_F11, KEY_F11},
   {SDLK_F12, KEY_F12},
   {SDLK_PRINTSCREEN, KEY_PRINTSCREEN},
   {SDLK_SCROLLLOCK, KEY_SCROLLLOCK},
   {SDLK_PAUSE, KEY_PAUSE},
   {SDLK_INSERT, KEY_INSERT},
   {SDLK_HOME, KEY_HOME},
   {SDLK_PAGEUP, KEY_PAGEUP},
   {SDLK_END, KEY_END},
   {SDLK_PAGEDOWN, KEY_PAGEDOWN},
   {SDLK_RIGHT, KEY_RIGHT},
   {SDLK_LEFT, KEY_LEFT},
   {SDLK_DOWN, KEY_DOWN},
   {SDLK_UP, KEY_UP},
   {SDLK_NUMLOCKCLEAR, KEY_NUMLOCKCLEAR},
   {SDLK_KP_DIVIDE, KEY_KP_DIVIDE},
   {SDLK_KP_MULTIPLY, KEY_KP_MULTIPLY},
   {SDLK_KP_MINUS, KEY_KP_MINUS},
   {SDLK_KP_PLUS, KEY_KP_PLUS},
   {SDLK_KP_ENTER, KEY_KP_ENTER},
   {SDLK_KP_1, KEY_KP_1},
   {SDLK_KP_2, KEY_KP_2},
   {SDLK_KP_3, KEY_KP_3},
   {SDLK_KP_4, KEY_KP_4},
   {SDLK_KP_5, KEY_KP_5},
   {SDLK_KP_6, KEY_KP_6},
   {SDLK_KP_7, KEY_KP_7},
   {SDLK_KP_8, KEY_KP_8},
   {SDLK_KP_9, KEY_KP_9},
   {SDLK_KP_0, KEY_KP_0},
   {SDLK_KP_PERIOD, KEY_KP_PERIOD},
   {SDLK_APPLICATION, KEY_APPLICATION},
   {SDLK_POWER, KEY_POWER},
   {SDLK_KP_EQUALS, KEY_KP_EQUALS},
   {SDLK_F13, KEY_F13},
   {SDLK_F14, KEY_F14},
   {SDLK_F15, KEY_F15},
   {SDLK_F16, KEY_F16},
   {SDLK_F17, KEY_F17},
   {SDLK_F18, KEY_F18},
   {SDLK_F19, KEY_F19},
   {SDLK_F20, KEY_F20},
   {SDLK_F21, KEY_F21},
   {SDLK_F22, KEY_F22},
   {SDLK_F23, KEY_F23},
   {SDLK_F24, KEY_F24},
   {SDLK_EXECUTE, KEY_EXECUTE},
   {SDLK_HELP, KEY_HELP},
   {SDLK_MENU, KEY_MENU},
   {SDLK_SELECT, KEY_SELECT},
   {SDLK_STOP, KEY_STOP},
   {SDLK_AGAIN, KEY_AGAIN},
   {SDLK_UNDO, KEY_UNDO},
   {SDLK_CUT, KEY_CUT},
   {SDLK_COPY, KEY_COPY},
   {SDLK_PASTE, KEY_PASTE},
   {SDLK_FIND, KEY_FIND},
   {SDLK_MUTE, KEY_MUTE},
   {SDLK_VOLUMEUP, KEY_VOLUMEUP},
   {SDLK_VOLUMEDOWN, KEY_VOLUMEDOWN},
   {SDLK_KP_COMMA, KEY_KP_COMMA},
   {SDLK_KP_EQUALSAS400, KEY_KP_EQUALSAS400},
   {SDLK_ALTERASE, KEY_ALTERASE},
   {SDLK_SYSREQ, KEY_SYSREQ},
   {SDLK_CANCEL, KEY_CANCEL},
   {SDLK_CLEAR, KEY_CLEAR},
   {SDLK_PRIOR, KEY_PRIOR},
   {SDLK_RETURN2, KEY_RETURN2},
   {SDLK_SEPARATOR, KEY_SEPARATOR},
   {SDLK_OUT, KEY_OUT},
   {SDLK_OPER, KEY_OPER},
   {SDLK_CLEARAGAIN, KEY_CLEARAGAIN},
   {SDLK_CRSEL, KEY_CRSEL},
   {SDLK_EXSEL, KEY_EXSEL},
   {SDLK_KP_00, KEY_KP_00},
   {SDLK_KP_000, KEY_KP_000},
   {SDLK_THOUSANDSSEPARATOR, KEY_THOUSANDSSEPARATOR},
   {SDLK_DECIMALSEPARATOR, KEY_DECIMALSEPARATOR},
   {SDLK_CURRENCYUNIT, KEY_CURRENCYUNIT},
   {SDLK_CURRENCYSUBUNIT, KEY_CURRENCYSUBUNIT},
   {SDLK_KP_LEFTPAREN, KEY_KP_LEFTPAREN},
   {SDLK_KP_RIGHTPAREN, KEY_KP_RIGHTPAREN},
   {SDLK_KP_LEFTBRACE, KEY_KP_LEFTBRACE},
   {SDLK_KP_RIGHTBRACE, KEY_KP_RIGHTBRACE},
   {SDLK_KP_TAB, KEY_KP_TAB},
   {SDLK_KP_BACKSPACE, KEY_KP_BACKSPACE},
   {SDLK_KP_A, KEY_KP_A},
   {SDLK_KP_B, KEY_KP_B},
   {SDLK_KP_C, KEY_KP_C},
   {SDLK_KP_D, KEY_KP_D},
   {SDLK_KP_E, KEY_KP_E},
   {SDLK_KP_F, KEY_KP_F},
   {SDLK_KP_XOR, KEY_KP_XOR},
   {SDLK_KP_POWER, KEY_KP_POWER},
   {SDLK_KP_PERCENT, KEY_KP_PERCENT},
   {SDLK_KP_LESS, KEY_KP_LESS},
   {SDLK_KP_GREATER, KEY_KP_GREATER},
   {SDLK_KP_AMPERSAND, KEY_KP_AMPERSAND},
   {SDLK_KP_DBLAMPERSAND, KEY_KP_DBLAMPERSAND},
   {SDLK_KP_VERTICALBAR, KEY_KP_VERTICALBAR},
   {SDLK_KP_DBLVERTICALBAR, KEY_KP_DBLVERTICALBAR},
   {SDLK_KP_COLON, KEY_KP_COLON},
   {SDLK_KP_HASH, KEY_KP_HASH},
   {SDLK_KP_SPACE, KEY_KP_SPACE},
   {SDLK_KP_AT, KEY_KP_AT},
   {SDLK_KP_EXCLAM, KEY_KP_EXCLAM},
   {SDLK_KP_MEMSTORE, KEY_KP_MEMSTORE},
   {SDLK_KP_MEMRECALL, KEY_KP_MEMRECALL},
   {SDLK_KP_MEMCLEAR, KEY_KP_MEMCLEAR},
   {SDLK_KP_MEMADD, KEY_KP_MEMADD},
   {SDLK_KP_MEMSUBTRACT, KEY_KP_MEMSUBTRACT},
   {SDLK_KP_MEMMULTIPLY, KEY_KP_MEMMULTIPLY},
   {SDLK_KP_MEMDIVIDE, KEY_KP_MEMDIVIDE},
   {SDLK_KP_PLUSMINUS, KEY_KP_PLUSMINUS},
   {SDLK_KP_CLEAR, KEY_KP_CLEAR},
   {SDLK_KP_CLEARENTRY, KEY_KP_CLEARENTRY},
   {SDLK_KP_BINARY, KEY_KP_BINARY},
   {SDLK_KP_OCTAL, KEY_KP_OCTAL},
   {SDLK_KP_DECIMAL, KEY_KP_DECIMAL},
   {SDLK_KP_HEXADECIMAL, KEY_KP_HEXADECIMAL},
   {SDLK_LCTRL, KEY_LCTRL},
   {SDLK_LSHIFT, KEY_LSHIFT},
   {SDLK_LALT, KEY_LALT},
   {SDLK_LGUI, KEY_LGUI},
   {SDLK_RCTRL, KEY_RCTRL},
   {SDLK_RSHIFT, KEY_RSHIFT},
   {SDLK_RALT, KEY_RALT},
   {SDLK_RGUI, KEY_RGUI},
   {SDLK_MODE, KEY_MODE},
   {SDLK_SLEEP, KEY_SLEEP},
   {SDLK_WAKE, KEY_WAKE},
   {SDLK_CHANNEL_INCREMENT, KEY_CHANNEL_INCREMENT},
   {SDLK_CHANNEL_DECREMENT, KEY_CHANNEL_DECREMENT},
   {SDLK_MEDIA_PLAY, KEY_MEDIA_PLAY},
   {SDLK_MEDIA_PAUSE, KEY_MEDIA_PAUSE},
   {SDLK_MEDIA_RECORD, KEY_MEDIA_RECORD},
   {SDLK_MEDIA_FAST_FORWARD, KEY_MEDIA_FAST_FORWARD},
   {SDLK_MEDIA_REWIND, KEY_MEDIA_REWIND},
   {SDLK_MEDIA_NEXT_TRACK, KEY_MEDIA_NEXT_TRACK},
   {SDLK_MEDIA_PREVIOUS_TRACK, KEY_MEDIA_PREVIOUS_TRACK},
   {SDLK_MEDIA_STOP, KEY_MEDIA_STOP},
   {SDLK_MEDIA_EJECT, KEY_MEDIA_EJECT},
   {SDLK_MEDIA_PLAY_PAUSE, KEY_MEDIA_PLAY_PAUSE},
   {SDLK_MEDIA_SELECT, KEY_MEDIA_SELECT},
   {SDLK_AC_NEW, KEY_AC_NEW},
   {SDLK_AC_OPEN, KEY_AC_OPEN},
   {SDLK_AC_CLOSE, KEY_AC_CLOSE},
   {SDLK_AC_EXIT, KEY_AC_EXIT},
   {SDLK_AC_SAVE, KEY_AC_SAVE},
   {SDLK_AC_PRINT, KEY_AC_PRINT},
   {SDLK_AC_PROPERTIES, KEY_AC_PROPERTIES},
   {SDLK_AC_SEARCH, KEY_AC_SEARCH},
   {SDLK_AC_HOME, KEY_AC_HOME},
   {SDLK_AC_BACK, KEY_AC_BACK},
   {SDLK_AC_FORWARD, KEY_AC_FORWARD},
   {SDLK_AC_STOP, KEY_AC_STOP},
   {SDLK_AC_REFRESH, KEY_AC_REFRESH},
   {SDLK_AC_BOOKMARKS, KEY_AC_BOOKMARKS},
   {SDLK_SOFTLEFT, KEY_SOFTLEFT},
   {SDLK_SOFTRIGHT, KEY_SOFTRIGHT},
   {SDLK_CALL, KEY_CALL},
   {SDLK_ENDCALL, KEY_ENDCALL} };

   return sdl_key_mapping[sdl_key];

}
