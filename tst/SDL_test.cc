#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>
#include <print>


#include "../src/input.hpp"

#define SDL_MAIN_HANDLED

// this is to abstract from SDL keypresses but it is kind of ugly actually (the pavel trick.)
static Key to_key(SDL_Event& event)
{
    static constexpr auto mapping = std::initializer_list<std::pair<uint32_t, Key>>{
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

static Mouse to_mouse(SDL_Event& event)
{

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

// argc and argv[] are necessary for SDL3 main compatibility trickery.
int main(int argc, char *argv[])
{
	static SDL_Window* window = nullptr;

    SDL_SetAppMetadata("tremble", "1.0", "com.example.renderer-clear");

    // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
    window = SDL_CreateWindow(
        "SDL3/OpenGL Demo", 1920, 1080, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an OpenGL context associated with the window.
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    // do some trickery with glad to actually load modern opengl.
    gladLoadGL();


    bool running = true;

    SDL_Event event;

    // Main loop
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

        glClearColor(1.0f,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Your rendering code goes here (e.g., drawing shapes, textures, etc.)


        // Present the current buffer to the screen
        SDL_GL_SwapWindow(window);

    }


    SDL_GL_DestroyContext(gl_context);

}