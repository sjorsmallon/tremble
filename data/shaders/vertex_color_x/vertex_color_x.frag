 #version 410

// outputs
layout(location = 0) out vec4 color_frag_out;

uniform vec4 color;

void main()
{
    color_frag_out = color;
}