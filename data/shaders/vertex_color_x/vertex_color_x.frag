 #version 410

// inputs
layout(location = 0) in vec3 barycentric_coordinates;


// outputs
layout(location = 0) out vec4 color_frag_out;


// uniforms
uniform vec4 color;

float brightness_based_on_barycentric_coordinates(vec3 barycentric_coordinates)
{
    // barycentric coordinates: if one of them is close to zero, that means we are near the "opposite"edge.
    float edge_factor = min(min(barycentric_coordinates.x, barycentric_coordinates.y), barycentric_coordinates.z);
        
    float threshold = 0.05f;  // from what point onwards does it get darker?

    // Create a brightness factor based on the edge proximity
    float brightness_factor;

    if (edge_factor < threshold)
    {
        // darken the color based on proximity to the edge
        // closer to the edge will have more influence on darkening
        // pick a value between 1.0 and 0.5, based on this value between 0.1).
        brightness_factor = mix(1.0, 0.5, (threshold - edge_factor) / threshold);
    } else {
        // Center is bright
        brightness_factor = 1.0;
    }

    return brightness_factor;
}

void main()
{
    float brightness = brightness_based_on_barycentric_coordinates(barycentric_coordinates);
    vec4 resulting_color = vec4(vec3(color)* brightness, color.a);
    color_frag_out = resulting_color;
}