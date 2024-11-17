#version 450

// Input from the vertex shader
in vec2 texture_coordinates_fs_in;

// Output color
out vec4 fragment_color;

// Texture sampler
uniform sampler2D base_texture;
uniform float tile_scale; // multiply the uvs by how much? by how much do you scale the uvs?

void main()
{

    vec2 scaled_uv = texture_coordinates_fs_in * tile_scale;
    scaled_uv = mod(scaled_uv, 1.f);
    // Sample the texture at the given UV coordinates
    // Flip the y-coordinate to correct upside-down texture
    vec4 texture_sample = texture(base_texture, scaled_uv);

    // Set the fragment color based on the texture texture_sample
    fragment_color = texture_sample;
}