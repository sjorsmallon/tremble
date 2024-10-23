#version 450


in vec2 frag_uv; // Interpolated UV coordinates from the vertex shader
in vec3 frag_position;

out vec4 frag_color; // Output color

uniform float line_thickness; // Thickness of the grid lines

void main()
{
    // let's first do a uv grid.
    float line_count = 10.0f;
    float test = 1.0f / 10.0f;
    float division_test = 1.0f / line_count;
    float v_partition = test;
    float u_partition = test;

    float distance_to_u_line = abs(mod(frag_uv.x, u_partition));
    float distance_to_v_line = abs(mod(frag_uv.y, v_partition));

    vec3 my_color = vec3(1.0f,0.0f,0.0f);

    if ((distance_to_v_line < 0.02f) || (distance_to_u_line < 0.02f))
    {
        my_color = vec3(1.0f, 1.0f, 1.0f);
    }
    else
    {
        discard;
    }




    // frag_color = vec4(distance_to_u_line, distance_to_v_line, 0.1f, 1.0f);
    frag_color = vec4(my_color, 1.0f);

}