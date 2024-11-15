#version 450

// Input vertex attributes
layout(location = 0) in vec3 position_vs_in;       // Vertex position in object space

// Uniform matrices
uniform mat4 model;       // Model (translation, rotation, scaling) matrix
uniform mat4 view;        // View matrix (for camera positioning)
uniform mat4 projection;  // Projection matrix (orthographic or perspective)

layout(location = 0) out vec3 barycentric_coordinates;;

void main()
{
     // Assigning barycentric coordinates to the vertices
    if (gl_VertexID % 3 == 0)
        barycentric_coordinates = vec3(1.0, 0.0, 0.0); // First vertex
    else if (gl_VertexID % 3 == 1)
        barycentric_coordinates = vec3(0.0, 1.0, 0.0); // Second vertex
    else // if (gl_VertexID % 3 == 2)
        barycentric_coordinates = vec3(0.0, 0.0, 1.0); // Third vertex

    // Apply model, view, and projection matrices in order
    gl_Position = projection * view * model * vec4(position_vs_in, 1.0);
}