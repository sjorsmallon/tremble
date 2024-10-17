clang -std=c++23 tst/client_server.cc -o -I inc/ build/test_client_server.exe -g & clang -std=c++23 tst/AABB_test.cc -o build/AABB_test.exe -g
clang -std=c++23 tst/SDL_baseline_test.cc -I inc/ -l external/static_libraries/SDL3.lib -lgdi32 -lopengl32 -o build/baseline_sdl_test.exe -g  -Wl,/SUBSYSTEM:WINDOWS
clang -std=c++23 tst/SDL_test.cc src/glad.cpp -I inc/ -l external/static_libraries/SDL3.lib -lgdi32 -lopengl32 -o build/sdl_test.exe -g  -Wl,/SUBSYSTEM:CONSOLE
