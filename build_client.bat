clang -std=c++23 -I inc/ src/main_client.cc src/ecs.cpp -o build/client.exe -l external/static_libraries/SDL3.lib -g