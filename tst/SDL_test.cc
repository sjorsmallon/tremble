#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_HANDLED

#include <windows.h>  // error box whatever.

#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <print>
#include <numeric> // std::accumulate
#include <array>

#include "../src/input.hpp"
#include "../src/gl_helpers.hpp"
#include "../src/AABB.hpp"
#include "../src/camera.hpp"
#include "../src/debug_draw.hpp"
#include "../src/bsp.hpp"
#include "../src/player_move.hpp"

// Macro to time a block of code or function call, and print function name
#define TIMED_FUNCTION(code_block) \
do { \
    Uint64 start = SDL_GetPerformanceCounter(); \
    code_block; \
    Uint64 end = SDL_GetPerformanceCounter(); \
    double elapsed_in_ms = (end - start) * 1000.0 / SDL_GetPerformanceFrequency(); \
    std::print("Function '{}' took: {} ms\n", __func__, elapsed_in_ms); \
} while(0)


// this is to abstract from SDL keypresses but it is kind of ugly actually (the pavel trick.)
static Key to_key(const SDL_Event& event)
{
    static constexpr auto mapping = std::initializer_list<std::pair<uint32_t, Key>> {
        {SDLK_W, Key::KEY_W},
        {SDLK_ESCAPE, Key::KEY_ESCAPE},
        {SDLK_A ,Key::KEY_A},
        {SDLK_S,Key::KEY_S},
        {SDLK_D,Key::KEY_D},
        {SDLK_P,Key::KEY_P},
    };

    for (auto& [key, value]: mapping)
    {
        if (key == event.key.key) // key.key???? zowie wowie!
        {
            return value;
        }
    }

    return Key::KEY_PLACEHOLDER;
}

static Mouse to_mouse(const SDL_Event& event)
{
    return Mouse::MOUSE_PLACEHOLDER;
}

// this needs access to camera.
void handle_keyboard_input(Key key)
{
    switch (key)
    {
        case Key::KEY_ESCAPE:
        {
            std::print("pressed Escape. See you!\n");
            exit(0);    
            break;
        }

        case Key::KEY_PLACEHOLDER:
        {
            std::print("pressed key that I do not know.\n");
            break;
        }

        // case Key::KEY_W:
        // {
        //     break;    
        // }


        // case Key::KEY_A:
        // {
        //     break;    
        // }


        // case Key::KEY_S:
        // {
        //     break;    
        // }

        // case Key::KEY_D:
        // {
        //     break;    
        // }

        default:
        {
            // std::print("unreachable.\n");
            break;            
        }
    }
}

// there is a better abstraction here that will come later.
static void draw_triangles(
    const uint32_t VAO,
    const uint32_t VBO,
    const size_t vertex_count,
    const uint32_t shader_program,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model
    )
{

    glUseProgram(shader_program);
    set_uniform(shader_program, "model", model);
    set_uniform(shader_program, "view", view);
    set_uniform(shader_program, "projection", projection);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

static void draw_lines(
    const uint32_t VAO,
    const uint32_t VBO,
    const size_t vertex_count,
    const uint32_t shader_program,
    const glm::mat4& projection,
    const glm::mat4& view
    )
{
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix (no transformation)
 
    glUseProgram(shader_program);
    set_uniform(shader_program, "model", model);
    set_uniform(shader_program, "view", view);
    set_uniform(shader_program, "projection", projection);

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, vertex_count);
}

uint32_t create_x_shader_program()
{
    const char* x_vertex_shader_src = R"(
    #version 330 core

    layout(location = 0) in vec3 position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0);
    }
    )";

    const char* x_fragment_shader_src = R"(
    #version 330 core

    out vec4 FragColor;

    void main() {
        // Hardcoded color
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); // RGB + Alpha (opacity)
    }
    )";

    auto x_shader_program = create_shader_program(
        x_vertex_shader_src,
        x_fragment_shader_src
    );

    return x_shader_program;
}

uint32_t create_interleaved_xnc_shader_program()
{
    const char* vertex_shader_src = R"(
        #version 410
        layout(location = 0) in vec3 position_vert_in;
        layout(location = 1) in vec3 normal_vert_in;
        layout(location = 2) in vec4 color_vert_in;

        
        layout(location = 0) out vec3 position_frag_in;
        layout(location = 1) out vec3 normal_frag_in;
        layout(location = 2) out vec4 color_frag_in;
        layout(location = 3) out vec3 barycentric;
        
        uniform mat4 model; // Model matrix
        uniform mat4 view;  // View matrix
        uniform mat4 projection; // Projection matrix

        void main()
        {
            // Assigning barycentric coordinates to the vertices
            if (gl_VertexID % 3 == 0)
                barycentric = vec3(1.0, 0.0, 0.0); // First vertex
            else if (gl_VertexID % 3 == 1)
                barycentric = vec3(0.0, 1.0, 0.0); // Second vertex
            else // if (gl_VertexID % 3 == 2)
                barycentric = vec3(0.0, 0.0, 1.0); // Third vertex

            normal_frag_in = mat3(transpose(inverse(model))) * normal_vert_in; // Transform the normal to world space
            position_frag_in = vec3(model * vec4(position_vert_in, 1.0)); // Transform the vertex position to world space
            color_frag_in = color_vert_in; // Pass vertex color to fragment shader

            gl_Position = projection * view * vec4(position_frag_in, 1.0); // Apply projection and view transformations
        })";

    const char* fragment_shader_src = R"(
        #version 410
        layout(location = 0) in vec3 position_frag_in;
        layout(location = 1) in vec3 normal_frag_in;
        layout(location = 2) in vec4 color_frag_in;
        layout(location = 3) in vec3 barycentric;

        layout(location = 0) out vec4 color_frag_out;

        float brightness_based_on_barycentric_coordinates(vec3 barycentric_coordinates)
        {
            // barycentric coordinates: if one of them is close to zero, that means we are near the "opposite"edge.
            float edge_factor = min(min(barycentric.x, barycentric.y), barycentric.z);
                
            float threshold = 0.05f;  // Adjust this value to control how dark the edges get

            // Create a brightness factor based on the edge proximity
            float brightness_factor;

            if (edge_factor < threshold)
            {
                // Darken the color based on proximity to the edge
                // Closer to the edge will have more influence on darkening
                // pick a value between 1.0 and 0.5, based on this value between 0.1).
                brightness_factor = mix(1.0, 0.5, (threshold - edge_factor) / threshold);
            } else {
                // Center is bright
                brightness_factor = 1.0;
            }

            return brightness_factor;
        }

        void main()
        {
            float brightness = brightness_based_on_barycentric_coordinates(barycentric);
            vec4 resulting_color = vec4(color_frag_in.xyz * brightness, 1.0);
            color_frag_out = resulting_color;
    })";

    auto shader_program = create_shader_program(
        vertex_shader_src,
        fragment_shader_src
    );
    return shader_program;
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
            "SDL3/OpenGL Demo", window_width, window_height, 
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
            SDL_WarpMouseInWindow(window, 0.f, 0.f); // xy 
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
        // std::print("WARNING: vsync is ON.\n");
        SDL_GL_SetSwapInterval(0);

        set_global_gl_settings();
    }    
    // shaders related
    uint32_t xnc_shader_program = create_interleaved_xnc_shader_program();
    uint32_t x_shader_program = create_x_shader_program();


    // timing related
    size_t current_timing_idx = 0;
    auto timings = std::array<double, 1024>{};

    // base AABB.
    auto path = std::string{"../data/list_of_AABB"};
    auto aabbs = read_AABBs_from_file(path);
    auto aabbs_vertices  = to_vertex_xnc(aabbs);
    auto aabb_gl_buffer = create_interleaved_xnc_buffer(aabbs_vertices);
    BSP* bsp = nullptr;
    {
        std::vector<uint64_t> face_indices{};
        int face_idx = 0;
        while (face_idx < aabbs_vertices.size())
        {
            face_indices.push_back(face_idx);
            face_idx += 3;
        }

        bsp = build_bsp(face_indices, aabbs_vertices);
    }

    // uv grid related.
    auto uv_grid_vertex_shader_str = file_to_string("../data/shaders/grid/grid.vert");
    auto uv_grid_fragment_shader_str = file_to_string("../data/shaders/grid/grid.frag");
    vec2 grid_world_space_dimensions = vec2{1000.f, 1000.f};
    auto uv_grid_vertices = generate_vertex_xu_quad_from_plane(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f,0.0f,1.0f}, 1000.0f, 1000.0f);

    auto uv_grid_shader_program = create_shader_program(
        uv_grid_vertex_shader_str.c_str(),
        uv_grid_fragment_shader_str.c_str()
    );

    if (!uv_grid_shader_program)
    {
        int result = MessageBox(NULL, "failed to compile uv grid shader.", "Error", MB_ICONERROR | MB_RETRYCANCEL);
        if (result == IDCANCEL)
        {
            SDL_Quit();
            return -1;
        }
    }

    set_uniform(uv_grid_shader_program, "line_thickness", 10.0f);
    set_uniform(uv_grid_shader_program, "grid_dimensions", vec2{1000.f, 1000.f});
    auto uv_grid_gl_buffer = create_interleaved_xu_buffer(uv_grid_vertices);


    // player related
    



    // shenanigans 
    bool first_frame = true;


    // game loop state
    bool running = true;
    SDL_Event event;

    // game state.
    auto camera = Camera{};
    camera.yaw= -90.f;
    camera.pitch = 0.f;

    float move_speed = 100.0f;
    float mouse_sensitivity = 2.0f;
    float fov = 90.0f;
    float near_z = 0.1f;
    float far_z = 4000.f;
    float dt = 0.f;
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t last = 0.f;
    float mouse_x{};
    float mouse_y{};
    float last_mouse_x{};
    float last_mouse_y{};
    // entity related
    vec3 player_velocity{};
    vec3 player_position{-6.0320406, 10, 580.2726};
    Move_Input move_input{};

    //@NOTE: SDL_GetPerformanceCounter is too high precision for floats. If I make it double "it just works". (SDL_GetPeformanceCOunter is actually uint64_t).
    while (running)
    {
         last = now;
         now = SDL_GetPerformanceCounter();
         dt = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) / 1000.0; // Convert to seconds

         last_mouse_x = mouse_x;
         last_mouse_y = mouse_y;

         while (SDL_PollEvent(&event))
        {
            // ignore all events except quit (alt-f4, pressing x, etc)
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;  // Break out of the loop to quit the application
            }

        }

        // handle keyboard input.
        {
            const bool* key_state = SDL_GetKeyboardState(NULL);
            if (key_state[SDL_SCANCODE_ESCAPE]) running = false;

            // Handle continuous key press movement
            move_input.forward_pressed = key_state[SDL_SCANCODE_W];
            move_input.backward_pressed = key_state[SDL_SCANCODE_S];
            move_input.left_pressed = key_state[SDL_SCANCODE_A];
            move_input.right_pressed = key_state[SDL_SCANCODE_D];
            move_input.jump_pressed = key_state[SDL_SCANCODE_SPACE];

            if (key_state[SDL_SCANCODE_P])
            {
                    std::print("player_position: {}\n", player_position);
                    std::print("camera.front: {}\n", camera.front);
                    std::print("camera.pitch: {}\n", camera.pitch);
            }

            // first, update the player position and velocity.
            glm::vec3 right = glm::cross(camera.front, camera.up);
            auto [new_position, new_velocity] = my_walk_move(
                move_input,
                player_position,
                player_velocity,
                vec3{camera.front.x, camera.front.y, camera.front.z},
                vec3{right.x, right.y, right.z},
                dt);

            player_position = new_position;
            player_velocity = new_velocity;

            camera.position = glm::vec3(new_position.x, new_position.y, new_position.z);

            vec3 position = vec3{player_position.x, player_position.y, player_position.z};
            size_t closest_face_idx = find_closest_proximity_face_index(bsp, aabbs_vertices, position, 2.5f);

        }

        // handle mouse input. this still seems jerky and is sometimes degenerate, where it locks the y axis. I don't understand why.
        {
            uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

            int dx = mouse_x - last_mouse_x;
            int dy = mouse_y - last_mouse_y;
            camera = look_around(camera, dx, dy, mouse_sensitivity);
        }

        // rendering code goes here.
        glClearColor(0.0f,0.05f,0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

        // aabbs.
        draw_triangles(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, xnc_shader_program,
            glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
            get_look_at_view_matrix(camera),
            glm::mat4(1.0f)
            );

        // vertex grid.
        // draw_lines(grid_gl_buffer.VAO, grid_gl_buffer.VBO, grid_gl_buffer.vertex_count, x_shader_program,
        //     glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
        //     get_look_at_view_matrix(camera)
        //     );

        // uv grid
        draw_triangles(uv_grid_gl_buffer.VAO, uv_grid_gl_buffer.VBO, uv_grid_gl_buffer.vertex_count, uv_grid_shader_program,
            glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
            get_look_at_view_matrix(camera),
            glm::mat4(1.0f)
            );

        SDL_GL_SwapWindow(window);

    }


    SDL_GL_DestroyContext(gl_context);

    return 0;
}