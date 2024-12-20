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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



// yuck
char SDL_Keycode_to_char(SDL_Keycode keycode, bool shift_pressed = false)
{
    static const std::unordered_map<SDL_Keycode, std::string> key_map = {
        { SDLK_A, "aA" }, { SDLK_B, "bB" }, { SDLK_C, "cC" }, { SDLK_D, "dD" },
        { SDLK_E, "eE" }, { SDLK_F, "fF" }, { SDLK_G, "gG" }, { SDLK_H, "hH" },
        { SDLK_I, "iI" }, { SDLK_J, "jJ" }, { SDLK_K, "kK" }, { SDLK_L, "lL" },
        { SDLK_M, "mM" }, { SDLK_N, "nN" }, { SDLK_O, "oO" }, { SDLK_P, "pP" },
        { SDLK_Q, "qQ" }, { SDLK_R, "rR" }, { SDLK_S, "sS" }, { SDLK_T, "tT" },
        { SDLK_U, "uU" }, { SDLK_V, "vV" }, { SDLK_W, "wW" }, { SDLK_X, "xX" },
        { SDLK_Y, "yY" }, { SDLK_Z, "zZ" },
        { SDLK_0, "0)" }, { SDLK_1, "1!" }, { SDLK_2, "2@" }, { SDLK_3, "3#" },
        { SDLK_4, "4$" }, { SDLK_5, "5%" }, { SDLK_6, "6^" }, { SDLK_7, "7&" },
        { SDLK_8, "8*" }, { SDLK_9, "9(" },
        { SDLK_SPACE, " " }, { SDLK_RETURN, "\n" }, { SDLK_TAB, "\t" },
        { SDLK_COMMA, ",<" }, { SDLK_PERIOD, ".>" }, { SDLK_SLASH, "/?" },
        { SDLK_SEMICOLON, ";:" }, { SDLK_APOSTROPHE, "'\"" }, { SDLK_LEFTBRACKET, "[{" },
        { SDLK_RIGHTBRACKET, "]}" }, { SDLK_BACKSLASH, "\\|" }, { SDLK_MINUS, "-_" },
        { SDLK_EQUALS, "=+" }, {SDLK_BACKSPACE, "\b\b"}
    };

    auto it = key_map.find(keycode);
    if (it != key_map.end()) {
        return shift_pressed ? it->second[1] : it->second[0];
    }

    // sentinel value: 0 if not found.
    return 0;
}

bool g_noclip = false;
void toggle_noclip()
{
    g_noclip = !g_noclip;
}

// argc and argv[] are necessary for SDL3 main compatibility trickery.
int main(int argc, char *argv[])
{
    int window_width = 1920;
    int window_height = 1080;
	SDL_Window* window = nullptr;
    SDL_GLContext gl_context{};
    {
        SDL_SetAppMetadata("tremble", "1.0", "com.example.renderer-clear");
        SDL_Init(SDL_INIT_VIDEO);
        // set gl attributes        
        {
            bool result;
            result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4): {}\n", result);
            result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5): {}\n", result);
            result = SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE): {}\n", result);
            result = SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 16);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_RED_SIZE,16): {}\n", result);      // 16 bits for red channel
            result = SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,16);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,16): {}\n", result);    // 16 bits for green channel
            result = SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,16);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,16): {}\n", result);     // 16 bits for blue channel
            result = SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,16);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,16): {}\n", result);    // 16 bits for alpha channel
            result = SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24): {}\n", result);   // 24 bits for depth buffer
            result = SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1): {}\n", result);  // Enable double buffering
            result = SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
            if (!result) std::print("SDL_Get_Error: {}\n", SDL_GetError());
            std::print("SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8): {}\n", result);  // 8 bits for stencil buffer
        }

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

        // do some trickery with glad to actually load modern opengl.
        gladLoadGL();

        // huh. even though I did not do this, it seems to not matter.
        SDL_GL_MakeCurrent(window, gl_context);

        // enable vsync
        SDL_GL_SetSwapInterval(1);

        set_global_gl_settings();
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
        Texture_Format::Red
        );

    // load the orange 128 * 128 texture.
    const char* texture_name = "../data/textures/orange_with_text.png";
    int x; 
    int y;
    int channels_in_file;
    int desired_channels = 4;
    uint8_t* data = stbi_load(texture_name, &x, &y, &channels_in_file, desired_channels);
    std::print("x: {}, y: {}, channels_in_file: {}\n", x, y, channels_in_file);
    std::vector<uint8_t> unfolded_data(data, data + ((x * y * channels_in_file) / sizeof(uint8_t)));
    auto wall_texture = register_texture_opengl(unfolded_data, x, y, Texture_Format::RGBA);
    auto wall_width = 1024;
    auto wall_height = 1024;
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
    constexpr auto max_character_count_in_string = 512;
    constexpr auto vertices_per_character = 6;
    std::vector<vertex_xu> text_character_vertices(max_character_count_in_string * vertices_per_character);
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
    BSP* world_map_bsp = build_bsp(world_map_vertices);

    // console geometry
    auto console_min = vec2{.x = 0.f, .y = 0.5f * static_cast<float>(window_height)};
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
    camera.yaw= -90.f;
    camera.pitch = 0.f;
    float noclip_move_speed = 500.0f;
    float mouse_sensitivity = 2.0f;
    float fov = 90.0f;
    float near_z = 0.1f;
    float far_z = 4000.f;
    vec3 world_up = vec3{0.f, 1.f, 0.f};

    // entity related
    vec3 player_velocity{};
    // vec3 player_position{-6.0320406, 10, 580.2726};
    vec3 player_position{};

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

    auto join_server_result_packet = Packet{};
    auto result = execute_with_timeout([&]() -> bool
    {
        if (client_socket.recv(join_server_result_packet, ipaddr) < 0) // blocking
        {
            print_network("[client] recv failed while waiting for the join_server result.\n");
        }
            else
            {
                if (join_server_result_packet.header.message_type == Message_Type::MESSAGE_JOIN_SERVER_ACCEPTED)
                {
                    print_network("[client] join server accepted!\n");
                    return true;
                }
            }

            return false;
        }, 1);

    if (result)
    {
        print_network("[client] playing online.\n");
        playing_online = true;
    }
    else
    {
        print_warning("playing offline\n");
    }

    while (running)
    {

        last = now;
        //@NOTE: SDL_GetPerformanceCounter is too high precision for floats. If I make it double "it just works". (SDL_GetPeformanceCOunter is actually uint64_t).
        now = SDL_GetPerformanceCounter();
        dt = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) / 1000.0; // Convert to seconds

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
                    char key = SDL_Keycode_to_char(event.key.key, shift_pressed);

                    if (key == '\b' && control_pressed)
                    {
                        // erase the input line
                        clear_input(console);
                        continue; // don't do anything else.
                    } 

                    // FIXME: this is not good enough! we need to also be able to pass up, down, left, right
                    // which do not have these ascii values.
                    if (key != 0) handle_keystroke(console, key); 

                    //@Note: for now, do it disjointed, so we do not tangle the systems at this point already.
                    if (key == '\n') {execute_command(command_system, console.history.back());}
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

        // handle keyboard input.
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

            // collision detection.
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
            auto [collider_planes, all_face_indices] = collect_and_classify_intersecting_planes(world_map_bsp, world_map_vertices, aabb);

            if (g_noclip)
            {
                face_indices = previous_face_indices;
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
                // auto result = execute_with_timeout([&]() -> bool
                // {
                    if (client_socket.recv(packet, ipaddr) < 0) // blocking
                    {
                        print_network("[client] recv failed while waiting for a position.\n");
                    }
                    else
                    {
                        // return true;
                    }
                        
                    // return false;
                // }, 5);

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