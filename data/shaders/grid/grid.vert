#version 450

layout(location = 0) in vec3 position; // Vertex position
layout(location = 1) in vec2 uv;       // UV coordinates

uniform mat4 model;     // Model matrix
uniform mat4 view;      // View matrix
uniform mat4 projection; // Projection matrix

out vec2 frag_uv; // Pass UV to fragment shader
out vec3 frag_position; // pass world space to fragment position.
void main()
{
    // Transform the vertex position to clip space
    gl_Position = projection * view * model * vec4(position, 1.0);

    // Pass UV coordinates to the fragment shader
    frag_uv = uv;
    // world space fragment coordinates
    frag_position = vec3(model * vec4(position, 1.0));
}
