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

        default:
        {
            std::print("unreachable.\n");
            break;            
        }
    }
}

static void draw_AABBS(const uint32_t VAO, const uint32_t VBO, const size_t vertex_count, const uint32_t shader_program)
{
    //@Hardcode: fov, window_width, window_height.
    int width = 1920;
    int height = 1080;

    glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 200.0f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)); // Move the camera back 3 units
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

	static SDL_Window* window = nullptr;
    SDL_GLContext gl_context{};
    {
        SDL_SetAppMetadata("tremble", "1.0", "com.example.renderer-clear");

        // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
        window = SDL_CreateWindow(
            "SDL3/OpenGL Demo", 1920, 1080, 
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
    auto path = std::string{"data/list_of_AABB"};
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

    auto default_triangle_vertices = create_default_triangle();
    auto default_triangle_buffer = create_interleaved_xnc_buffer(default_triangle_vertices);

    const char* base_vertex_shader_src = R"(
               #version 330 core
        layout(location = 0) in vec3 aPos;     // Vertex position
        layout(location = 1) in vec3 aNormal;  // Vertex normal (not used in this simple shader)
        layout(location = 2) in vec4 aColor;    // Vertex color

        out vec4 FragColor; // Output color to fragment shader

        void main() {
            gl_Position = vec4(aPos, 1.0); // Transform to clip space
            FragColor = aColor;             // Pass color to fragment shader
        })";

    const char* base_fragment_shader_src = R"(
        #version 330 core
        in vec4 FragColor; // Input color from vertex shader
        out vec4 finalColor; // Final color output

        void main() {
            finalColor = FragColor; // Set the output color
    })";

    auto base_shader_program = create_shader_program(
        base_vertex_shader_src,
        base_fragment_shader_src
        );

    bool running = true;
    SDL_Event event;
    while (running)
    {
        // Process events
        while (SDL_PollEvent(&event))
        {

            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                auto key = to_key(event);
                handle_keyboard_input(key);
            }

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            // Additional event handling can go here (e.g., input, window events)
        }

        glClearColor(0.0f,0.2f,0.0f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

        // rendering code goes here.
        draw_AABBS(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, shader_program);

        // Present the current buffer to the screen
        SDL_GL_SwapWindow(window);

        SDL_Delay(100); 

    }


    SDL_GL_DestroyContext(gl_context);

    return 0;
}