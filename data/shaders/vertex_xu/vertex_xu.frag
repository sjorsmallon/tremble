#version 450

// Input from the vertex shader
in vec2 texture_coordinates_fs_in;

// Output color
out vec4 fragment_color;

// Texture sampler
uniform sampler2D textTexture;
uniform vec3 color;

void main()
{
    // Sample the texture at the given UV coordinates
    // vec4 sampledColor = texture(textTexture, TexCoord);
    
    // Set the fragment color based on the texture sample
    fragment_color = vec4(color, 0.5f);
}