#version 450

// Input from the vertex shader
in vec2 texture_coordinates_fs_in;

// Output color
out vec4 fragment_color;

// Texture sampler
uniform sampler2D text_bitmap;
uniform vec3 color;

void main()
{
    // Sample the texture at the given UV coordinates
    // Flip the y-coordinate to correct upside-down texture
    float intensity = texture(text_bitmap, texture_coordinates_fs_in).r;
    
    // Set the fragment color based on the texture sample
    fragment_color = vec4(vec3(intensity, intensity, intensity), 1.f);
}