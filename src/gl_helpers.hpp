#pragma once
#include <glad/glad.h>
#include <vector>


#include "../src/vertex.hpp"
#include "vec.hpp"

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

        // set clear color to WHITE.
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
    }
}


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
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(vertex_xnc), values.data(), GL_STATIC_DRAW);

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

// Shader stuff.
bool check_program_link_status(GLuint program) 
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
    
    return true; 
}

bool check_shader_compile_status(GLuint shader)
{
    GLint success{}; 
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); 
    
    if (!success)
    { 
        char info_log[512]; 
        glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log);
        std::print("ERROR::SHADER::COMPILATION_FAILED. openGL error {}\n",info_log); 
        return false;
    }
    
    return true;
}

struct Shader_Program
{
    uint32_t id;
};

uint32_t create_shader_program(
    const char* vertex_shader_source,
    const char* fragment_shader_source)
{
    // Create vertex shader
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);
    if (!check_shader_compile_status(vertex_shader))
    {
        glDeleteShader(vertex_shader);
        return 0; 
    }

    // Create fragment shader
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
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

template <typename Type>
inline void set_uniform(GLuint program_id, const std::string_view name, const Type& value)
{
    GLint location = glGetUniformLocation(program_id, name.data());
    if (location == -1) // sentinel value for not existing.
    {
         std::print("Uniform '{}' does not exist in the shader program.\n", name);;
    }

    // Set the uniform based on its type
    if constexpr (std::is_same_v<Type, vec3>)
    {
        glUniform3fv(location, 1, &value);
    }
    else if constexpr (std::is_same_v<Type, vec4>)
    {
        glUniform4fv(location, 1, &value);
    } else if constexpr (std::is_same_v<Type, glm::mat4>)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    } else {
        static_assert(std::is_same_v<Type, vec3> || std::is_same_v<Type, vec4> || std::is_same_v<Type, glm::mat4>,
                      "Unsupported uniform type.");
    }
}


