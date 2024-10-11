#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SDL_MAIN_HANDLED

// argc and argv[] are necessary for SDL
int main(int argc, char *argv[])
{
	static SDL_Window* window = nullptr;
	static SDL_Renderer* renderer = nullptr;

    SDL_SetAppMetadata("tremble", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("AAAAAAAAAA", 640, 480, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }



	// Main loop flag
    bool running = true;

    // Event structure
    SDL_Event event;

    // Main loop
    while (running) {
        // Process events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            // Additional event handling can go here (e.g., input, window events)
        }

        // Clear the screen (e.g., with black color)
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Your rendering code goes here (e.g., drawing shapes, textures, etc.)

        // Present the current buffer to the screen
        SDL_RenderPresent(renderer);
    }

}