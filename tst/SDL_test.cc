#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_HANDLED

#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <print>

#include "../src/input.hpp"
#include "../src/gl_helpers.hpp"
#include "../src/AABB.hpp"
#include "../src/camera.hpp"
#include "../src/debug_draw.hpp"

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
static void draw_vertex_xnc_buffer(
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

static void draw_lines_vertex_x_buffer(
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
        // Hardcoded line color (white)
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

        // Enable relative mouse mode and capture the mouse
        SDL_CaptureMouse(true);
        SDL_HideCursor();

        // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
        window = SDL_CreateWindow(
            "SDL3/OpenGL Demo", window_width, window_height, 
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
      
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        // Create an OpenGL context associated with the window.
        gl_context = SDL_GL_CreateContext(window);
        // do some trickery with glad to actually load modern opengl.
        gladLoadGL();

        // huh. even though I did not do this, it seems to not help.
        SDL_GL_MakeCurrent(window, gl_context);

        // disable vsync
        SDL_GL_SetSwapInterval(0);
    }
    
    set_global_gl_settings();


    // drawing related stuff.
    auto path = std::string{"../data/list_of_AABB"};
    auto aabbs = read_AABBs_from_file(path);
    auto vertices  = to_vertex_xnc(aabbs);
    auto aabb_gl_buffer = create_interleaved_xnc_buffer(vertices);

    uint32_t xnc_shader_program = create_interleaved_xnc_shader_program();
    uint32_t x_shader_program = create_x_shader_program();

    float grid_size = 1000.0f;
    float grid_spacing = 10.0f;

    // to be interpreted as (start, end).
    auto grid_vertices = generate_grid_lines_from_plane(vec3{0.0f,0.0f,0.0f}, vec3{0.0f, 1.0f, 0.0f}, grid_size, grid_spacing);
    auto grid_gl_buffer=  create_x_buffer(grid_vertices);

    auto arrow_vertices = generate_arrow_vertices(vec3{0.0f, 0.0f, 10.0f}, {100.0f, 0.0f, 10.0f}, 10.0f);
    auto arrow_gl_buffer = create_interleaved_xnc_buffer(arrow_vertices);

    auto x_arrow_vertices = generate_arrow_vertices(vec3{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.15f);
    auto x_arrow_gl_buffer = create_interleaved_xnc_buffer(x_arrow_vertices);

    auto y_arrow_vertices = generate_arrow_vertices(vec3{0.0f, 0.f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0.15f);
    auto y_arrow_gl_buffer = create_interleaved_xnc_buffer(y_arrow_vertices);

    auto z_arrow_vertices = generate_arrow_vertices(vec3{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0.15f);
    auto z_arrow_gl_buffer = create_interleaved_xnc_buffer(z_arrow_vertices);
    

    bool running = true;
    SDL_Event event;

    // game state.
    auto camera = Camera{};    
    float move_speed = 100.0f;
    float mouse_sensitivity = 2.0f;
    float fov = 90.0f;
    float near_z = 0.1f;
    float far_z = 500.f;
    float dt = 0.f;
    double now = SDL_GetPerformanceCounter();
    double last = 0.f;
    float mouse_x;
    float mouse_y;
    float last_mouse_x;
    float last_mouse_y;

    //@NOTE: SDL_GetPerformanceCounter is too high precision for floats. If I make it double "it just works".
    while (running)
    {
         last = now;
         now = SDL_GetPerformanceCounter();
         dt = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) / 1000.0; // Convert to seconds

         last_mouse_x = mouse_x;
         last_mouse_y = mouse_y;

        // Process events
        while (SDL_PollEvent(&event))
        {
            //Note: this is _not good enough_ to deal with repeated keystrokes. we need to poll the keyboard state every frame. 
            // if (event.type == SDL_EVENT_KEY_DOWN)
            // {
            //     auto key = to_key(event);
            //     handle_keyboard_input(key);

            //     // temporary debug stuff.
            //     if (key == Key::KEY_P)
            //     {
            //         std::print("{}",camera.position);
            //     }
            // }

            // if (event.type == SDL_EVENT_QUIT)
            // {
            //     running = false;
            // }

            // if (event.type == SDL_EVENT_MOUSE_MOTION)
            // {
            //     // auto& mouse_move = event.motion;
            //     // float dx = mouse_move.xrel;
            //     // float dy = mouse_move.yrel;
            //     // 
            // }
            // Additional event handling can go here (e.g., input, window events)
        }


        // handle keyboard input.
        {
            const bool* key_state = SDL_GetKeyboardState(NULL);
            // Handle continuous key press movement
            bool move_forward = key_state[SDL_SCANCODE_W];
            bool move_backward = key_state[SDL_SCANCODE_S];
            bool move_left = key_state[SDL_SCANCODE_A];
            bool move_right = key_state[SDL_SCANCODE_D];
            camera = update_camera(camera, dt, move_forward, move_left, move_backward, move_right, move_speed);

            if (key_state[SDL_SCANCODE_ESCAPE]) running = false;
        }

        // handle mouse input. this still seems jerky and is sometimes degenerate, where it locks the y axis. I don't understand why.
        {
            uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
            int dx = mouse_x - last_mouse_x;
            int dy = mouse_y - last_mouse_y;
            camera = look_around(camera, dx, dy, mouse_sensitivity);
        }

        // rendering code goes here.
        glClearColor(0.0f,0.2f,0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

        // aabbs.
        // draw_vertex_xnc_buffer(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, xnc_shader_program,
        //     glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
        //     get_look_at_view_matrix(camera),
        //     glm::mat4(1.0f)
        //     );

        // just the one arrow 
        // draw_vertex_xnc_buffer(arrow_gl_buffer.VAO, arrow_gl_buffer.VBO, arrow_gl_buffer.vertex_count, xnc_shader_program,
        //     glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
        //     get_look_at_view_matrix(camera),
        //     glm::mat4(1.0f)
        //     );

        // the three arrows.
        // think about glm::ortho.
        // stop guessing.
        glDisable(GL_DEPTH_TEST);


        // think about it differently.
        // projection is the same.
        // the view is ok.
        // the main thing is this: we need to translate it to the center of NDC. how do we do that?
        // the translation is the translation in world space. I am confusing myself.

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z);
        glm::mat4 view = get_look_at_view_matrix(camera);
        auto right = normalize(cross(camera.front, camera.up));
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), camera.position + 2.0f * normalize(camera.front) + 1.0f * normalize(right));


        draw_vertex_xnc_buffer(x_arrow_gl_buffer.VAO, x_arrow_gl_buffer.VBO, x_arrow_gl_buffer.vertex_count, xnc_shader_program,
            projection,
            view,
            translation
        );
        
        draw_vertex_xnc_buffer(y_arrow_gl_buffer.VAO, y_arrow_gl_buffer.VBO, y_arrow_gl_buffer.vertex_count, xnc_shader_program,
        projection,
        view,
        translation
        );
        
        draw_vertex_xnc_buffer(z_arrow_gl_buffer.VAO, z_arrow_gl_buffer.VBO, z_arrow_gl_buffer.vertex_count, xnc_shader_program,
        projection,
        view,
        translation
        );
        glEnable(GL_DEPTH_TEST);

        draw_lines_vertex_x_buffer(grid_gl_buffer.VAO, grid_gl_buffer.VBO, grid_gl_buffer.vertex_count, x_shader_program,
            glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
            get_look_at_view_matrix(camera)
            );

        SDL_GL_SwapWindow(window);

    }


    SDL_GL_DestroyContext(gl_context);

    return 0;
}