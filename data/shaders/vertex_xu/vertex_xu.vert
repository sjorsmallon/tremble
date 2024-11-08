#version 450

// Input vertex attributes
layout(location = 0) in vec3 position_vs_in;       // Vertex position in object space
layout(location = 1) in vec2 texture_coordinates_vs_in;  // Texture coordinates

// Uniform matrices
uniform mat4 model;       // Model (translation, rotation, scaling) matrix
uniform mat4 view;        // View matrix (for camera positioning)
uniform mat4 projection;  // Projection matrix (orthographic or perspective)

// Output to the fragment shader
out vec2 texture_coordinates_fs_in;

void main()
{
    // Apply model, view, and projection matrices in order
    gl_Position = projection * view * model * vec4(position_vs_in, 1.0);

    // Pass the texture coordinates to the fragment shader
    texture_coordinates_fs_in = texture_coordinates_vs_in;
}