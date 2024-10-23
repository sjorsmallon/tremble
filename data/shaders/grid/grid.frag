#version 450

precision highp float;

in vec2 frag_uv; // Interpolated UV coordinates from the vertex shader
in vec3 frag_position;

out vec4 frag_color; // Output color

uniform float line_thickness; // Thickness of the grid lines
uniform vec2 grid_dimensions;

void main()
{

    float horizontal_line_count = grid_dimensions.x / line_thickness;
    float vertical_line_count = grid_dimensions.y / line_thickness;

    // let's first do a uv grid.

    float test = 1.0f / 10.0f;
    float segment = 1.0f / horizontal_line_count;
    float v_partition = segment;
    float u_partition = segment;

    float distance_to_u_line = abs(mod(frag_uv.x, u_partition));
    float distance_to_v_line = abs(mod(frag_uv.y, v_partition));

    vec3 my_color = vec3(distance_to_u_line , distance_to_v_line, 0.0f);

    if ((distance_to_v_line < 0.001f) || (distance_to_u_line < 0.001f))
    {
        my_color = vec3(1.0f, 1.0f, 1.0f);
    }
    else
    {
        discard;
    }



    frag_color = vec4(my_color, 1.0f);

}