#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>
#include <print>

#define SDL_MAIN_HANDLED

//TODO: figure out why this window completely detaches from the console?

// argc and argv[] are necessary for SDL3 main compatibility trickery.
int main(int argc, char *argv[])
{
	static SDL_Window* window = nullptr;
    SDL_SetAppMetadata("tremble", "1.0", "com.example.renderer-clear");


    // Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
    window = SDL_CreateWindow(
        "SDL3/OpenGL Demo", 640, 480, 
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create an OpenGL context associated with the window.
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    // do some fuckery with glad to actually load modern opengl.
    gladLoadGL();

	// Main loop flag
    bool running = true;

    // Event structure
    SDL_Event event;

    // Main loop
    while (running)
    {

        // Process events
        while (SDL_PollEvent(&event))
        {
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