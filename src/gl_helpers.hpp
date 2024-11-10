#pragma once
#include <glad/glad.h>
#include <vector>

#include "debug_draw.hpp"
#include "vertex.hpp"
#include "vec.hpp"
#include <iostream>
#include <fstream>


// move this to file.
std::string file_to_string(const std::string& filename)
{
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string content;
    content.resize(size);

    if (!file.read(&content[0], size))
    {
        throw std::runtime_error("Error reading file: " + filename);
    }

    return content;
}

// openGL error callback
void GLAPIENTRY opengl_message_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam )
{
    static constexpr auto warnings_to_ignore = std::initializer_list<int32_t>{
        0x8251,  /// 0x8251: bound to video memory (intended.)
        0x8250   /// 0x8250: buffer performance warning: copying atomic buffer
    };


    bool warning_can_be_ignored = true;
    for (auto& warning: warnings_to_ignore)
    {
        if (type == warning)
        {
            warning_can_be_ignored = false;
        }    
    }

    if (!warning_can_be_ignored) 
    {
        if (type == GL_DEBUG_TYPE_ERROR)
        {
                std::print("[error] GL CALLBACK: type = 0x{:x}, severity = 0x{:x}, message = {}\n", type, severity, message);
        }
        else
        {
            std::print("GL CALLBACK: type = 0x{:x}, severity = 0x{:x}, message = {}\n", type, severity, message);
        }
    }
}

inline void set_global_gl_settings()
{
   //----modify global openGL state
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Enable use of the z-buffer
        glEnable(GL_DEPTH_TEST); // default is GL_LESS

        // Line width
        glLineWidth(2.0);
        glEnable(GL_LINE_SMOOTH);

        // enable debug output
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(opengl_message_callback, 0);

        // DISABLE transparency
        glDisable(GL_BLEND);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_FRAMEBUFFER_SRGB);
    }
}

// Shader stuff.
inline bool check_program_link_status(GLuint program) 
{
    GLint success{}; 
    glGetProgramiv(program, GL_LINK_STATUS, &success); 

    if (!success) 
    {
        char info_log[512]; 
        glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log); 
        std::print("ERROR::PROGRAM::LINKING_FAILED\n{}\n", info_log); 
        return false; 
    }
    
    std::print("program linked succesfully.\n");

    return true; 
}

inline bool check_shader_compile_status(GLuint shader)
{
    GLint success{}; 
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); 
    
    if (!success)
    { 
        char info_log[512]; 
        glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
        std::print("ERROR::SHADER::COMPILATION_FAILED. openGL error: {}\n",info_log); 
        return false;
    }
    
    std::print("shader compiled succesfully.\n");

    return true;
}


uint32_t create_shader_program(
    const char* vertex_shader_source,
    const char* fragment_shader_source)
{
    // Create vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    std::print("compiling Vertex shader.", vertex_shader_source);
    glCompileShader(vertex_shader);
    if (!check_shader_compile_status(vertex_shader))
    {
        glDeleteShader(vertex_shader);
        return 0; 
    }

    // Create fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    std::print("compiling Fragment shader.", fragment_shader_source);
    glCompileShader(fragment_shader);
    if (!check_shader_compile_status(fragment_shader))
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;  // Failed to compile fragment shader
    }

    // Create shader program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    // Check for linking errors
    if (!check_program_link_status(program)) {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        glDeleteProgram(program);
        return 0;  // Failed to link program
    }

    // Clean up shaders (they are now linked into the program and no longer needed separately)
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    // Shader shader{.id = program};
    return program;
}


//@Note: we can do some real shit if we compile this thing beforehand,
// and store the uniforms already.
// we are inconsistent here with vec3/vec4 and glm::mat4. we should probably consolidate at some point.
// I dislike how glm seeps into every inch of your program.
#include <glm/gtc/type_ptr.hpp> 
#include <glm/glm.hpp>  // For GLM vector types and functions
#include <glm/gtc/type_ptr.hpp>  // For convenient functions like value_ptr


///WARNING::::: PLEASE BE CAREFUL WHEN ADDING SOMETHING NEW TO THIS THING. DO NOT COPY PASTE,
// MAKE SURE YOU USE THE RIGHT GLUNIFORM FUNCTION, AND THE RIGHT TYPE (EITHER GLM OR NOT).
template <typename Type>
inline void set_uniform(GLuint program_id, const std::string_view name, const Type& value)
{
    GLint location = glGetUniformLocation(program_id, name.data());
    if (location == -1) // sentinel value for not existing.
    {
         std::print("Uniform '{}' does not exist in the shader program.\n", name);;
    }
    glUseProgram(program_id);

    // Set the uniform based on its type
    if constexpr (std::is_same_v<Type, int>)
    {
        glUniform1i(location, value);
    }
    else if constexpr (std::is_same_v<Type, float>)
    {
        glUniform1f(location, value);        
    }
    else if constexpr (std::is_same_v<Type, vec2>) 
    {
        glUniform2fv(location, 1, (float*)&value);
    }
    else if constexpr (std::is_same_v<Type, vec3>) 
    {
        glUniform3fv(location, 1, (float*)&value);
    }
    else if constexpr (std::is_same_v<Type, vec4>)
    {
        glUniform4fv(location, 1, &value);
    } else if constexpr (std::is_same_v<Type, glm::mat4>)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    } else {
        static_assert(std::is_same_v<Type, vec3> || std::is_same_v<Type, vec3> || std::is_same_v<Type, vec4> || std::is_same_v<Type, glm::mat4>,
                      "Unsupported uniform type.");
    }
}

void print_shader_uniforms(GLuint shader_program)
{
    GLint uniform_count;
    glGetProgramiv(shader_program, GL_ACTIVE_UNIFORMS, &uniform_count);

    std::print("Active Uniforms: {}\n", uniform_count);

    for (GLint i = 0; i < uniform_count; ++i)
    {
        char uniform_name[256];
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveUniform(shader_program, i, sizeof(uniform_name), &length, &size, &type, uniform_name);

        GLint location = glGetUniformLocation(shader_program, uniform_name);

        // Print the uniform information
        std::print("Uniform #{}: {}\n", i, uniform_name);
        std::print("  Location: {}\n", location);
        std::print("  Size: {}\n", size);
        std::print("  Type: 0x{:X}\n", type);  // Print the type in hexadecimal (GL_FLOAT, GL_INT, etc.)
    }
}

//------------------------------------------------------------------------
// stuff below this line is just me spitballing. everything above it can be yoinked and placed 
// in any other program with minor changes (std::print, glm::vec3.)

struct GL_Buffer
{
    uint32_t VAO;
    uint32_t VBO;
    uint32_t vertex_count;
};

// FIXME: actually, I like that we had a vector of float instead of vector of xnc. can we still fix this?
// in the future. maybe.

GL_Buffer create_x_buffer(const std::vector<vec3>& values)
{
    GL_Buffer gl_buffer{};

    // Generate and bind VAO and VBO
    glGenVertexArrays(1, &gl_buffer.VAO); 
    glGenBuffers(1, &gl_buffer.VBO);
    glBindVertexArray(gl_buffer.VAO);

    // Fill the buffer with vertex data (position only)
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(vec3), values.data(), GL_STATIC_DRAW); // yikes, that was wrong.

    // Attribute IDs and layout for position
    const int32_t position_attribute_id = 0;
    const int32_t position_float_count = 3;
    const int32_t position_byte_offset = 0;
    const int32_t vertex_byte_stride = position_float_count * sizeof(float);

    // Set the vertex count
    gl_buffer.vertex_count = values.size();

    // Enable and specify the position attribute (vec3)
    glEnableVertexAttribArray(position_attribute_id);
    glVertexAttribPointer(
        position_attribute_id,
        position_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)position_byte_offset
    );

    // Unbind the buffer and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return gl_buffer;
}

GL_Buffer create_interleaved_xu_buffer(const std::vector<vertex_xu>& values)
{
    GL_Buffer gl_buffer{};

    // Generate and bind VAO and VBO
    glGenVertexArrays(1, &gl_buffer.VAO); 
    glGenBuffers(1, &gl_buffer.VBO);
    glBindVertexArray(gl_buffer.VAO);

    // Fill the buffer with vertex data (position only)
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(vertex_xu), values.data(), GL_STATIC_DRAW);

    // Attribute IDs and layout for position
    const int32_t position_attribute_id = 0;
    const int32_t uv_attribute_id = 1;

    const int32_t position_float_count = 3;
    const int32_t uv_float_count = 2;

    const int32_t position_byte_offset = 0;
    const int32_t uv_byte_offset = position_float_count * sizeof(float);

    const int32_t vertex_float_count = position_float_count + uv_float_count;
    const int32_t vertex_byte_stride = vertex_float_count * sizeof(float);

    // Set the vertex count
    gl_buffer.vertex_count = values.size();

    // Enable and specify the position attribute (vec3)
    glEnableVertexAttribArray(position_attribute_id);
    glVertexAttribPointer(
        position_attribute_id,
        position_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)position_byte_offset
    );

    glEnableVertexAttribArray(uv_attribute_id);
    glVertexAttribPointer(
        uv_attribute_id,
        uv_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)uv_byte_offset
    );

    // Unbind the buffer and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return gl_buffer;
}

GL_Buffer create_interleaved_xnc_buffer(const std::vector<vertex_xnc>& interleaved_xnc_values)
{
    GL_Buffer gl_buffer{};

    glGenVertexArrays(1, &gl_buffer.VAO); 
    glGenBuffers(1,      &gl_buffer.VBO);
    glBindVertexArray(gl_buffer.VAO);

    // fill buffer with vertices.
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.VBO);
    glBufferData(GL_ARRAY_BUFFER, interleaved_xnc_values.size() * sizeof(vertex_xnc), interleaved_xnc_values.data(), GL_STATIC_DRAW);

    // actually, I kind of like what I was doing here. I refactored it in a minor fashion for a different type and I did not really mind.
    const int32_t position_attribute_id = 0;
    const int32_t normal_attribute_id   = 1;
    const int32_t color_attribute_id    = 2;

    const int32_t position_float_count  = 3;
    const int32_t normal_float_count    = 3;
    const int32_t color_float_count     = 4;

    const int32_t position_byte_offset  = 0; // 0
    const int32_t normal_byte_offset    =  position_float_count * sizeof(float); // 3
    const int32_t color_byte_ofset      = (position_float_count + normal_float_count) * sizeof(float); // 6

    const int32_t vertex_float_count    = position_float_count + normal_float_count + color_float_count; // (3 + 3 + 4 = 10)
    const int32_t vertex_byte_stride    = vertex_float_count * sizeof(float); // oh. I wonder if this can cause giant issues with padding?

    // set vertex count.
    gl_buffer.vertex_count = interleaved_xnc_values.size(); 

    // position vertex attribute
    glEnableVertexAttribArray(position_attribute_id);
    glVertexAttribPointer(
        position_attribute_id,
        position_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)position_byte_offset
        );

    // normal vertex attribute
    glEnableVertexAttribArray(normal_attribute_id);
    glVertexAttribPointer(
        normal_attribute_id,
        normal_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)normal_byte_offset
        );

    // texture vertex attribute
    glEnableVertexAttribArray(color_attribute_id);
    glVertexAttribPointer(
        color_attribute_id,
        color_float_count,
        GL_FLOAT,
        GL_FALSE,
        vertex_byte_stride,
        (void*)color_byte_ofset
        );

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return gl_buffer;
}

struct Shader_Program
{
    uint32_t id;
};

// we are ignoring z because this is intended to be used for pixel coordinates. 
std::vector<vertex_xu> generate_vertex_xu_quad(const vec2& min, const vec2& max)
{
    // this should be in draw_console, but I need to think about what we need to do.
    // draw transparent half-screen quad. this is in pixels.
    vertex_xu top_left     = vertex_xu{.position = vec3{.x = min.x, .y = max.y, .z =  0}, .uv = vec2{.u = 0.f, .v = 1.f}};
    vertex_xu top_right    = vertex_xu{.position = vec3{.x = max.x, .y = max.y, .z =  0}, .uv = vec2{.u = 1.f, .v = 1.f}};
    vertex_xu bottom_left  =  vertex_xu{.position = vec3{.x = min.x, .y = min.y, .z =  0}, .uv = vec2{.u = 0.f, .v = 0.f}};
    vertex_xu bottom_right =  vertex_xu{.position = vec3{.x = max.x, .y = min.y, .z =  0}, .uv = vec2{.u = 1.f, .v = 0.f}};

    auto vertices = std::vector<vertex_xu>{
        top_left,
        bottom_left,
        bottom_right,
        top_left,
        bottom_right,
        top_right
    };

    return vertices;
}


// we're at the point where maybe we should generalize this. is it possible to do this at compile time? by looking at the types?
std::vector<vertex_xnc> create_default_triangle()
{
    std::vector<vertex_xnc> triangle;

    // Define vertices with positions in NDC
    triangle.push_back({
        .position = vec3{-1.0f, -1.0f, 0.0f}, // Bottom-left (Red)
        .normal = vec3{0.0f, 0.0f, 1.0f},     // Normal pointing up
        .color = vec4{1.0f, 0.0f, 0.0f, 1.0f} // Red
    });

    triangle.push_back({
        .position = vec3{1.0f, -1.0f, 0.0f},  // Bottom-right (Blue)
        .normal = vec3{0.0f, 0.0f, 1.0f},     // Normal pointing up
        .color = vec4{0.0f, 0.0f, 1.0f, 1.0f} // Blue
    });

    triangle.push_back({
        .position = vec3{0.0f, 1.0f, 0.0f},   // Top (Green)
        .normal = vec3{0.0f, 0.0f, 1.0f},     // Normal pointing up
        .color = vec4{0.0f, 1.0f, 0.0f, 1.0f} // Green
    });

    return triangle;
}


// Function to generate a quad defined by a plane's normal and position
std::vector<vertex_xu> generate_vertex_xu_quad_from_plane(const vec3& center, const vec3& normal, float width, float height) {
    std::vector<vertex_xu> vertices(6);  // A quad has 6 vertices

    // Calculate two orthogonal vectors (tangent and bitangent) on the plane
    vec3 tangent, bitangent;

    // If the normal is too close to the up axis, use a different reference vector
    vec3 up = vec3{0.0f, 1.0f, 0.0f};
    if (abs(dot(normal, up)) > 0.99f) {
        up = vec3{1.0f, 0.0f, 0.0f};  // Use X axis instead if too close to Y
    }

    tangent = normalize(cross(up, normal));
    bitangent = normalize(cross(normal, tangent));

    float half_width = width * 0.5f;
    float half_height = height * 0.5f;

    // Now calculate the positions of the quad's 4 vertices in world space
    // Top-left (in local space relative to the center)
    auto top_left = vertex_xu{.position = center + (-half_width * tangent) + (half_height * bitangent), .uv = vec2{0.0f, 1.0f}};

    // Top-right
    auto top_right = vertex_xu{.position = center + (half_width * tangent) + (half_height * bitangent), .uv = vec2{1.0f, 1.0f}};

    // Bottom-right
    auto bottom_right = vertex_xu{.position = center + (half_width * tangent) + (-half_height * bitangent), .uv = vec2{1.0f, 0.0f}};

    // Bottom-left
    auto bottom_left = vertex_xu{.position = center + (-half_width * tangent) + (-half_height * bitangent),. uv = vec2{0.0f, 0.0f}};

    vertices[0] = top_left;
    vertices[1] = bottom_right;
    vertices[2] = top_right; 

    vertices[3] = top_left;
    vertices[4] = bottom_left;
    vertices[5] = bottom_right;

    return vertices;
}

inline uint32_t create_x_shader_program()
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

inline uint32_t create_interleaved_xnc_shader_program()
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


// for later.
struct Texture_Config
{
    bool generate_mipmaps;
    bool use_anisotropic_filtering;
};

enum class Texture_Format {
    Red,
    RGB,
    RGBA,
    Depth,
};

struct GL_Texture
{
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    Texture_Format format;
    uint32_t texture_unit; // the "window frame" thing. this needs to be communicated to the shader.
    // configuration? what attributes are active?
};

// no texture packing, just assign each texture its own activev texture (window frame).
GL_Texture create_texture(Texture_Format format, std::vector<uint8_t>& data, int width, int height)
{
    static int texture_unit_counter = 0;  // Static counter for texture units (glActiveTexture(GL_TEXTURE_0 + texture_unit_counter))

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    GLenum gl_format;
    GLenum gl_internal_format;

    switch (format) {
        case Texture_Format::Red:
            gl_format = GL_RED;
            gl_internal_format = GL_RED;
            break;
        case Texture_Format::RGB:
            gl_format = GL_RGB;
            gl_internal_format = GL_RGB8;
            break;
        case Texture_Format::RGBA:
            gl_format = GL_RGBA;
            gl_internal_format = GL_RGBA8;
            break;
        case Texture_Format::Depth:
            gl_format = GL_DEPTH_COMPONENT;
            gl_internal_format = GL_DEPTH_COMPONENT32;
            break;
        default:
            gl_format = GL_RGBA;
            gl_internal_format = GL_RGBA8;
            break;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, GL_UNSIGNED_BYTE, data.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glActiveTexture(GL_TEXTURE0 + texture_unit_counter);  // Activate the texture unit
    glBindTexture(GL_TEXTURE_2D, texture);  // Bind the texture to the active texture unit
    texture_unit_counter += 1;

    assert(texture_unit_counter < 15 && "ran out of active textures.\n");


    GL_Texture gl_texture{};
    gl_texture.handle = texture;
    gl_texture.width = width;
    gl_texture.height = height;
    gl_texture.format = format;
    gl_texture.texture_unit = texture_unit_counter;



    return gl_texture;
}


// ---- drawing

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
    glDisable(GL_CULL_FACE);

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




void debug_draw_arrow(const vec3& start, const vec3& end, float diameter, const vec3& color)
{
    static bool first_time = true;
    static uint32_t arrow_shader_program = 0;
    static GL_Buffer gl_buffer; // Reusable VAO/VBO buffer

    auto vertices = generate_arrow_vertices(start, end, diameter);

    if (first_time)
    {
        arrow_shader_program = create_interleaved_xnc_shader_program();
        gl_buffer = create_interleaved_xnc_buffer(vertices); // Initialize with an empty vector
        first_time = false;
    }

    // Update the VBO with new vertices
    glBindVertexArray(gl_buffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(vertex_xnc), vertices.data());

    // Use the shader program
    glUseProgram(arrow_shader_program);

    // Set any necessary uniforms for the shader (e.g., color, transformations, etc.)
    set_uniform(arrow_shader_program, "color", color);

    // Draw the arrow
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Unbind the buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void debug_draw_triangles(
    const std::vector<vertex_xnc>& vertices,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& model)
{
    static bool first_time = true;
    static uint32_t basic_shader_program = 0;
    static GL_Buffer gl_buffer; // Reusable VAO/VBO buffer

    if (first_time)
    {
        basic_shader_program = create_interleaved_xnc_shader_program();
        gl_buffer = create_interleaved_xnc_buffer(vertices); // Initialize with an empty vector
        first_time = false;
    }

    // Update the VBO with new vertices
    glBindVertexArray(gl_buffer.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, gl_buffer.VBO);

    // Allocate or resize the VBO buffer based on the size of vertices
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex_xnc), vertices.data(), GL_DYNAMIC_DRAW);

    // Use the shader program
    glUseProgram(basic_shader_program);

    set_uniform(basic_shader_program, "model", model);
    set_uniform(basic_shader_program, "view", view);
    set_uniform(basic_shader_program, "projection", projection);

    // Draw the triangles
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    // Unbind the buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}