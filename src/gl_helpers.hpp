#pragma once
#include <glad/glad.h>
#include <vector>

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
        glLineWidth(1.0);
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

#include "../src/vertex.hpp"

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
    gl_buffer.vertex_count = interleaved_xnc_values.size() / vertex_float_count; 

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