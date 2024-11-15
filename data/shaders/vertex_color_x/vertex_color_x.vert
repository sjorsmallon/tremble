#version 450

// Input vertex attributes
layout(location = 0) in vec3 position_vs_in;       // Vertex position in object space

// Uniform matrices
uniform mat4 model;       
uniform mat4 view;        
uniform mat4 projection;  

void main()
{
 
    // Apply model, view, and projection matrices in order
    gl_Position = projection * view * model * vec4(position_vs_in, 1.0);
}