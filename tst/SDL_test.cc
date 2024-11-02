#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#define SDL_MAIN_HANDLED

#include <windows.h>  // error box whatever.

#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

#include <unordered_set>

#include <print>
#include <array>
// #include <chrono>
#include "../src/input.hpp"
#include "../src/gl_helpers.hpp"
#include "../src/AABB.hpp"
#include "../src/camera.hpp"
#include "../src/debug_draw.hpp"
#include "../src/bsp.hpp"
#include "../src/player_move.hpp"


#define TIMEIT(stmt) do { \
    auto start = std::chrono::high_resolution_clock::now(); \
    stmt; \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start); \
    std::print("{} took {:.3f} ms\n", #stmt, diff.count() / 1000.0); \
} while(0)

// there is a better abstraction here that will come later.
static void draw_triangles(
    const uint32_t VAO,
    const uint32_t VBO,
    const size_t vertex_count,
    const uint32_t shader_program,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model,
    bool wireframe
    )
{
    if (wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    // glDisable(GL_CULL_FACE);

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
    // uint32_t x_shader_program = create_x_shader_program();

    // base AABB.
    auto path = std::string{"../data/list_of_AABB"};
    auto aabbs = read_AABBs_from_file(path);
    auto aabbs_vertices  = to_vertex_xnc(aabbs);
    // i do not trust auto assignment.
    std::vector<vertex_xnc> base_aabbs_vertices = aabbs_vertices;
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
    auto uv_grid_vertices = generate_vertex_xu_quad_from_plane(vec3{0.0f, 0.0f, 0.0f}, vec3{0.0f,0.0f,1.0f}, grid_world_space_dimensions.x, grid_world_space_dimensions.y);

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
    set_uniform(uv_grid_shader_program, "grid_dimensions", grid_world_space_dimensions);
    auto uv_grid_gl_buffer = create_interleaved_xu_buffer(uv_grid_vertices);


    // game loop state
    bool noclip = false;
    bool running = true;
    SDL_Event event;

    // game state.
    auto camera = Camera{};
    camera.yaw= -90.f;
    camera.pitch = 0.f;

    float noclip_move_speed = 500.0f;
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
    auto player_aabb = AABB{.min = vec3{-20.0f, -20.0f, -20.0f}, .max = {20.0f, 45.f, 20.f}};
    auto player_aabb_vertices = to_vertex_xnc(player_aabb);
    auto player_aabb_gl_buffer = create_interleaved_xnc_buffer(player_aabb_vertices);

    Move_Input move_input{};
    Trace trace{};

    std::vector<size_t> previous_face_indices{};
    glm::mat4 aabb_transform_matrix(1.0f); // so we can render the "last" colliding hitbox so I can do some manual inspection.
    while (running)
    {
        // reset trace.
        trace.collided = false;
        trace.face_normal = vec3{0.0f, 0.0f, 0.0f};

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
                running = false;  // Break out of the loop to quit the application
            }

            if (event.type == SDL_EVENT_KEY_UP)
            {
                if ((event.key.key == SDLK_TILDE) || (event.key.key == SDLK_GRAVE))
                {
                    // toggle noclip.                   
                    noclip = 1 - noclip;
                    std::print("noclip status: {}\n", (noclip ? "on" : "off"));
                    // set the player velocity in the camera viewing direction.
                    glm::vec3 vel = camera.front * noclip_move_speed;
                    player_velocity = vec3{vel.x, vel.y, vel.z}; 

                    glm::vec3 position = glm::vec3(player_position.x, player_position.y, player_position.z);
                    // transformation matrix.
                    aabb_transform_matrix = glm::translate(glm::mat4(1.0f), position);
                }
            }

        }

        // handle keyboard input.
        {
            const bool* key_state = SDL_GetKeyboardState(NULL);
            if (key_state[SDL_SCANCODE_ESCAPE]) running = false;

            // Handle continuous key press movement
            move_input.forward_pressed  = key_state[SDL_SCANCODE_W];
            move_input.backward_pressed = key_state[SDL_SCANCODE_S];
            move_input.left_pressed     = key_state[SDL_SCANCODE_A];
            move_input.right_pressed    = key_state[SDL_SCANCODE_D];
            move_input.jump_pressed     = key_state[SDL_SCANCODE_SPACE];

            if (key_state[SDL_SCANCODE_P])
            {
                    std::print("player_position: {}\n", player_position);
                    std::print("camera.front: {}\n", camera.front);
                    std::print("camera.pitch: {}\n", camera.pitch);
            }

            // collision detection.
            std::vector<size_t> face_indices;
            {

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


                // update player_aabb to world space.
                auto aabb = AABB{.min = player_position + player_aabb.min, .max = player_position + player_aabb.max};
                auto [ground_face_indices, non_ground_face_indices] = bsp_trace_AABB(bsp, aabb, aabbs_vertices);

                if (!noclip)
                {


                    //@Note: there are duplicates. because we do not split faces upon bsp construction.
                    // that means we need to filter them out somewhere. I do that here. This is kind of annoying.
                    // we should think of something better.
                    auto filter_duplicates = [](const std::vector<size_t>& vec) {
                        std::unordered_set<size_t> seen;
                        std::vector<size_t> unique_elements;

                        for (size_t value : vec) {
                            if (seen.find(value) == seen.end()) {
                                seen.insert(value); 
                                unique_elements.push_back(value);
                            }
                        }
                        return unique_elements;
                    };

                    ground_face_indices = filter_duplicates(ground_face_indices);
                    non_ground_face_indices = filter_duplicates(non_ground_face_indices);

                    // temporary. this logic needs to be cleaned up.
                    face_indices.reserve(ground_face_indices.size() + non_ground_face_indices.size());
                    face_indices.insert(face_indices.end(), ground_face_indices.begin(), ground_face_indices.end());
                    face_indices.insert(face_indices.end(), non_ground_face_indices.begin(), non_ground_face_indices.end());
                }

                if (noclip)
                {
                    face_indices = previous_face_indices;
                }

                // color the intersecting face indices white.
                for (auto& face_idx: face_indices)
                {

                    std::array<vertex_xnc, 3> intersecting_face{
                        aabbs_vertices[face_idx],
                        aabbs_vertices[face_idx + 1],
                        aabbs_vertices[face_idx + 2]
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
            }

            if (!noclip)
            {
                // first, update the player position and velocity.
                glm::vec3 right = glm::cross(camera.front, camera.up);

                // using traces is misguided. but it is nice to know what the ground trace is / if we have a ground trace. I guess. I will just pass in the set of planes. construct them here.
                // auto traces = AABB_Traces{};

                // plane is combination of v0 and the calculated normal.
                std::vector<Plane> collider_planes{};
                for (auto& face_idx:face_indices)
                {
                    collider_planes.push_back(Plane{aabbs_vertices[face_idx].position, compute_triangle_normal(aabbs_vertices[face_idx].position, aabbs_vertices[face_idx + 1].position, aabbs_vertices[face_idx + 2].position)});
                }


                auto [new_position, new_velocity] = player_move(
                    move_input,
                    collider_planes,
                    player_position,
                    player_velocity,
                    vec3{camera.front.x, camera.front.y, camera.front.z},
                    vec3{right.x, right.y, right.z},
                    dt);

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

        //@Note: mouse input is no longer degenerate, it was caused by dt being 0 because of narrowing to float.
        {
            uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
            if (!mouse_state) //@Note this is just to stop the whining about mouse_state being unused.
            {

            }
            int dx = mouse_x - last_mouse_x;
            int dy = mouse_y - last_mouse_y;
            camera = look_around(camera, dx, dy, mouse_sensitivity);
        }

        // rendering code goes here.
        {
            glClearColor(0.0f,0.05f,0.0f, 1); // nsight says this is not allowed?
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // DO NOT FORGET TO CLEAR THE DEPTH BUFFER! it will yield just a black screen otherwise.

            // aabbs.


            draw_triangles(aabb_gl_buffer.VAO, aabb_gl_buffer.VBO, aabb_gl_buffer.vertex_count, xnc_shader_program,
                glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                get_look_at_view_matrix(camera),
                glm::mat4(1.0f),
                false // wireframe
                );

            if (noclip)
            {
                // not only toggle noclip, also draw the aabb at the last position.
                draw_triangles(player_aabb_gl_buffer.VAO, player_aabb_gl_buffer.VBO, player_aabb_gl_buffer.vertex_count, xnc_shader_program,
                glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                get_look_at_view_matrix(camera),
                aabb_transform_matrix,
                false //wireframe
                );
            }

            if (noclip)
            {

                auto construct_faces = [](const std::vector<vertex_xnc>& aabbs_vertices, const std::vector<size_t>& face_indices) {
                    std::vector<vertex_xnc> faces;
                    for (size_t face_idx = 0; face_idx < face_indices.size(); ++face_idx)
                    {
                            auto& v0 = aabbs_vertices[face_indices[face_idx]];
                            auto& v1 = aabbs_vertices[face_indices[face_idx] + 1];
                            auto& v2 = aabbs_vertices[face_indices[face_idx] + 2];
                            faces.push_back(v0);   
                            faces.push_back(v1);
                            faces.push_back(v2);
                            auto normal = compute_triangle_normal(v0.position, v1.position, v2.position);
                    }

                    return faces; // Return the constructed vector of faces
                };

                auto faces = construct_faces(aabbs_vertices, previous_face_indices);
                if (faces.size() > 0)
                {
                    debug_draw_triangles(
                        faces, 
                    glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
                    get_look_at_view_matrix(camera),
                    glm::mat4(1.0f));

                }
            }

            // uv grid
            // draw_triangles(uv_grid_gl_buffer.VAO, uv_grid_gl_buffer.VBO, uv_grid_gl_buffer.vertex_count, uv_grid_shader_program,
            //     glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
            //     get_look_at_view_matrix(camera),
            //     glm::mat4(1.0f)
            //     );

            // player bounding box.
            // vec3 target_position = player_position + (20.f * vec3{camera.front.x, camera.front.y, camera.front.z});

            // draw_triangles(player_aabb_gl_buffer.VAO, player_aabb_gl_buffer.VBO, player_aabb_gl_buffer.vertex_count, xnc_shader_program,
            // glm::perspective(glm::radians(fov), (float)window_width / (float)window_height, near_z, far_z),
            // get_look_at_view_matrix(camera),
            // glm::translate(glm::mat4(1.0f), glm::vec3(target_position.x, target_position.y, target_position.z))  
            // );
        }

        SDL_GL_SwapWindow(window);
    }


    SDL_GL_DestroyContext(gl_context);

    return 0;
}