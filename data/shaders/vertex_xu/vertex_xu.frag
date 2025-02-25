#version 450

// Input from the vertex shader
in vec2 texture_coordinates_fs_in;

// Output color
out vec4 fragment_color;

// Texture sampler
uniform sampler2D base_texture;
uniform vec3 color;

void main()
{
    // Sample the texture at the given UV coordinates
    // Flip the y-coordinate to correct upside-down texture
    vec4 texture_sample = texture(base_texture, texture_coordinates_fs_in);
    // Set the fragment color based on the texture texture_sample
    fragment_color = texture_sample;
}