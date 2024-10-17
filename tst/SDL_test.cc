#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_HANDLED

#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <print>

#include "../src/input.hpp"
#include "../src/gl_helpers.hpp"
#include "../src/AABB.hpp"
#include "../src/camera.hpp"

// this is to abstract from SDL keypresses but it is kind of ugly actually (the pavel trick.)
static Key to_key(const SDL_Event& event)
{
    static constexpr auto mapping = std::initializer_list<std::pair<uint32_t, Key>> {
        {SDLK_W, Key::KEY_W},
        {SDLK_ESCAPE, Key::KEY_ESCAPE},
        {SDLK_A ,Key::KEY_A},
        {SDLK_S,Key::KEY_S},
        {SDLK_D,Key::KEY_D},
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

static void draw_AABBS(
    const uint32_t VAO,
    const uint32_t VBO,
    const size_t vertex_count,
    const uint32_t shader_program,
    const glm::mat4& projection,
    const glm::mat4& view
    )
{
    //@Hardcode: fov, window_width, window_height.
    int width = 1920;
    int height = 1080;

    // glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 200.0f);
    // glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)); // Move the camera back 3 units
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix (no transformation)
 
    glUseProgram(shader_program);
    set_uniform(shader_program, "model", model);
    set_uniform(shader_program, "view", view);
    set_uniform(shader_program, "projection", projection);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
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
    }
    
    set_global_gl_settings();


    // drawing related stuff.
    auto path = std::string{"../data/list_of_AABB"};
    auto aabbs = read_AABBs_from_file(path);
    auto vertices  = to_vertex_xnc(aabbs);
    auto aabb_gl_buffer = create_interleaved_xnc_buffer(vertices);

    const char* vertex_shader_src = R"(
        #version 410
        layout(location = 0) in vec3 position_vert_in;
        layout(location = 1) in vec3 normal_vert_in;
        layout(location = 2) in vec4 color_vert_in;

        
        layout(location = 0) out vec3 position_frag_in;
        layout(location = 1) out vec3 normal_frag_in;
        layout(location = 2) out vec4 color_frag_in;
        
        uniform mat4 model; // Model matrix
        uniform mat4 view;  // View matrix
        uniform mat4 projection; // Projection matrix

        void main()
        {
            position_frag_in = vec3(model * vec4(position_vert_in, 1.0)); // Transform the vertex position to world space
            normal_frag_in = mat3(transpose(inverse(model))) * normal_vert_in; // Transform the normal to world space
            gl_Position = projection * view * vec4(position_frag_in, 1.0); // Apply projection and view transformations
            color_frag_in = color_vert_in; // Pass vertex color to fragment shader
        })";

    const char* fragment_shader_src = R"(
        #version 410
        layout(location = 0) in vec3 position_frag_in;
        layout(location = 1) in vec3 normal_frag_in;
        layout(location = 2) in vec4 color_frag_in;

        layout(location = 0) out vec4 color_frag_out;

        void main() {
            color_frag_out = color_frag_in;
    })";

    auto shader_program = create_shader_program(
        vertex_shader_src,
        fragment_shader_src
    );

    bool running = true;
    SDL_Event event;

    // game state.
    auto camera = Camera{};    
    float move_speed = 10.0f;
    float mouse_sensitivity = 2.0f;
    float dt = 0.f;
    float now = SDL_GetPerformanceCounter();
    float last = 0.f;

    while (running)
    {
         last = now;
         now = SDL_GetPerformanceCounter();
         dt = (double)((now - last) * 1000 / (double)SDL_GetPerformanceFrequency()) / 1000.0; // Convert to seconds

        // Process events
        while (SDL_PollEvent(&event))
        {
            //Note: this is _not_good_enough_ to deal with repeated keystrokes. we need to poll the keyboard state every frame. 
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                auto key = to_key(event);
                handle_keyboard_input(key);
            }

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_EVENT_MOUSE_MOTION)
            {
                auto& mouse_move = event.motion;
                float dx = mouse_move.xrel;
                float dy = mouse_move.yrel;

                camera = look_around(camera, dx, dy, mouse_sensitivity);
            }
            // Additional event handling can go here (e.g., input, window events)
        }

         // Get the current key state
        const bool* key_state = SDL_GetKeyboardState(NULL);

        // Handle continuous key press movement
        bool move_forward = key_state[SDL_SCANCODE_W];
        bool move_backward = key_state[SDL_SCANCODE_S];
        bool move_left = key_state[SDL_SCANCODE_A];
        bool move_right = key_state[SDL_SCANCODE_D];

        // Update camera based on the key state
        camera = update_camera(camera, dt, move_forward, move_left, move_backward, move_right, move_speed);



        // rendering code goes here.
        glClearColor(0.0f,0.2f,0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

        draw_AABBS(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, shader_program,
            glm::perspective(glm::radians(60.0f), (float)window_width / (float)window_height, 0.1f, 500.0f),
            get_view_matrix(camera)
            );



        // Present the current buffer to the screen
        SDL_GL_SwapWindow(window);

        // SDL_Delay(100); 

    }


    SDL_GL_DestroyContext(gl_context);

    return 0;
}