#pragma once
#include <cmath>
#include <vector>

#include "vertex.hpp"
#include "vec.hpp"
// actually, it is kind of awful that we store color at every vertex.
// open question: is it misguided to generate vertices for specific worldspace, instead of providing a model matrix or something like that?
// i don't care.
inline std::vector<vertex_xnc> generate_arrow_vertices(const vec3& start,const vec3& end,const float radius)
{
    std::vector<vertex_xnc> vertices;

    // Arrow head starts at 80%, and is twice as wide as the radius.
    const int slice_count = 20; // Number of vertices for each circular slice (shaft and arrowhead)
    const float pi = 3.14159265358979323846f;

    // 1. Calculate the direction and length of the arrow
    vec3 direction = normalize(end - start);
    float arrow_length = length(end - start);
    
    // Arrow shaft ends at 80% of the length
    float shaft_length = 0.7f * arrow_length;
    vec3 shaft_end = start + direction * shaft_length;

    // Arrowhead starts at the shaft end
    float arrowhead_radius = 1.5f * radius;  // Arrowhead is twice as wide as the shaft
   
    //@FIXME: why is this not used?
    //float arrowhead_length = arrow_length - shaft_length; //-> this scales as a factor of length. I dislike this I guess. I would it to scale as a factor of the radius. too much thinking.

    // 2. calculate tangent and bitangent axes.
    vec3 tangent{};
   	vec3 bitangent{};
    if (abs(direction.z) > 0.999f)
    {
        // If the direction is aligned with the Z-axis, use a different basis
        tangent = vec3{1.0f, 0.0f, 0.0f};  // Choose X-axis for tangential direction
        bitangent = vec3{0.0f, 1.0f, 0.0f}; // Use Y-axis for bitangent direction
    } else {
        // Compute orthogonal vectors for non-Z-aligned directions
        tangent = normalize(vec3{direction.y, -direction.x, 0.0f});  // Orthogonal to direction
        bitangent = cross(direction, tangent);  // Bitangent completes the orthogonal frame
    }


    // 3. Generate vertices for the shaft (cylinder)
    for (int i = 0; i < slice_count; ++i) {
        float angle = (2.0f * pi * i) / slice_count;
        float next_angle = (2.0f * pi * (i + 1)) / slice_count;

        // Compute the offset in 3D for circular slices, using tangent and bitangent vectors
        vec3 offset = tangent * (std::cos(angle) * radius) + bitangent * (std::sin(angle) * radius);
        vec3 next_offset = tangent * (std::cos(next_angle) * radius) + bitangent * (std::sin(next_angle) * radius);

        vec3 base_pos = start + offset;
        vec3 next_base_pos = start + next_offset;

        // Invert winding order for the shaft
        vertices.push_back({ base_pos, normalize(offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ next_base_pos, normalize(next_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + offset, normalize(offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });

        vertices.push_back({ shaft_end + offset, normalize(offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ next_base_pos, normalize(next_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + next_offset, normalize(next_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
    }

    // 4. Close off the base of the arrowhead
    // Add triangles between the last slice of the shaft and the base of the arrowhead
    for (int i = 0; i < slice_count; ++i) {
        float angle = (2.0f * pi * i) / slice_count;
        float next_angle = (2.0f * pi * (i + 1)) / slice_count;

        // Offsets for both the shaft and the arrowhead base (in 3D)
        vec3 shaft_offset = tangent * (std::cos(angle) * radius) + bitangent * (std::sin(angle) * radius);
        vec3 next_shaft_offset = tangent * (std::cos(next_angle) * radius) + bitangent * (std::sin(next_angle) * radius);
        vec3 arrowhead_offset = tangent * (std::cos(angle) * arrowhead_radius) + bitangent * (std::sin(angle) * arrowhead_radius);
        vec3 next_arrowhead_offset = tangent * (std::cos(next_angle) * arrowhead_radius) + bitangent * (std::sin(next_angle) * arrowhead_radius);

        // Correct winding to ensure counter-clockwise (CCW) order
        vertices.push_back({ shaft_end + shaft_offset, normalize(shaft_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + next_shaft_offset, normalize(next_shaft_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + arrowhead_offset, normalize(arrowhead_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });

        vertices.push_back({ shaft_end + arrowhead_offset, normalize(arrowhead_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + next_shaft_offset, normalize(next_shaft_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
        vertices.push_back({ shaft_end + next_arrowhead_offset, normalize(next_arrowhead_offset), vec4{1.0f, 1.0f, 1.0f, 1.0f} });
    }

    // 5. Generate vertices for the arrowhead (cone)
    vec3 arrow_tip = end;

    for (int i = 0; i < slice_count; ++i)
    {
        float angle = (2.0f * pi * i) / slice_count;
        float next_angle = (2.0f * pi * (i + 1)) / slice_count;

        // Compute the offset for the base of the arrowhead in 3D
        vec3 offset = tangent * (std::cos(angle) * arrowhead_radius) + bitangent * (std::sin(angle) * arrowhead_radius);
        vec3 next_offset = tangent * (std::cos(next_angle) * arrowhead_radius) + bitangent * (std::sin(next_angle) * arrowhead_radius);

        // Correct winding for the arrowhead cone
        vec3 base_pos = shaft_end + offset;
        vec3 next_base_pos = shaft_end + next_offset;
        vec3 normal = normalize(cross(next_base_pos - arrow_tip, base_pos - arrow_tip));  // Normal for the cone surface

        // Add the triangles for the cone's sides (with correct CCW winding)
        vertices.push_back({ base_pos, normal, vec4{1.0f, 0.0f, 0.0f, 1.0f} });  // Base vertex
        vertices.push_back({ next_base_pos, normal, vec4{1.0f, 0.0f, 0.0f, 1.0f} });  // Next base vertex
        vertices.push_back({ arrow_tip, normal, vec4{1.0f, 0.0f, 0.0f, 1.0f} });  // Tip of the arrow
    }


  	// 6. Generate end cap for the shaft
    vec3 end_cap_center = start;  // Center of the cap is the start of the shaft
    for (int i = 0; i < slice_count; ++i) {
        float angle = (2.0f * pi * i) / slice_count;
        float next_angle = (2.0f * pi * (i + 1)) / slice_count;

        // Compute the offsets for the cap
        vec3 offset = tangent * (std::cos(angle) * radius) + bitangent * (std::sin(angle) * radius);
        vec3 next_offset = tangent * (std::cos(next_angle) * radius) + bitangent * (std::sin(next_angle) * radius);

        // The normal for the end cap is the opposite of the direction
        vec3 normal = -direction;

        // Invert the winding for the end cap
        vertices.push_back({ end_cap_center, normal, vec4{1.0f, 1.0f, 1.0f, 1.0f} });  // Center vertex of the cap
        vertices.push_back({ end_cap_center + next_offset, normal, vec4{1.0f, 1.0f, 1.0f, 1.0f} });  // Next outer vertex
        vertices.push_back({ end_cap_center + offset, normal, vec4{1.0f, 1.0f, 1.0f, 1.0f} });  // Current outer vertex
    }

    return vertices;
}


std::vector<vec3> generate_grid_lines_from_plane(vec3 position, vec3 plane_normal, float grid_size, float grid_spacing)
{
    std::vector<vec3> lines{};
    vec3 grid_center = vec3{position.x, position.y, position.z};
    vec3 normal = normalize(plane_normal);  // Ensure normal is unit length

    // Find two perpendicular vectors to the normal to define the grid's axes
    vec3 u_axis{};
    vec3 v_axis{};
    if (abs(normal.z) < 0.999)
    {
        u_axis = normalize(cross(vec3{normal.x, normal.y, normal.z}, vec3{0.0f, 0.0f, 1.0f})); // Cross with Z-axis
    } else
    {
        u_axis = normalize(cross(vec3{normal.x, normal.y, normal.z}, vec3{1.0f, 0.0f, 0.0f})); // Special case for Z normal
    }
    v_axis = normalize(cross(vec3{normal.x, normal.y, normal.z}, u_axis));  // v-axis is orthogonal to u_axis and normal

    // Define the grid size and spacing
    for (float i = -grid_size; i <= grid_size; i += grid_spacing)
    {
        vec3 start = grid_center + i * v_axis - grid_size * u_axis;
        vec3 end   = grid_center + i * v_axis + grid_size * u_axis;

        lines.push_back(vec3{start.x, start.y, start.z});
        lines.push_back(vec3{end.x, end.y, end.z});
    }

    // Draw lines along the 'v' axis
    for (float i = -grid_size; i <= grid_size; i += grid_spacing)
    {
        vec3 start = grid_center + i * u_axis - grid_size * v_axis;
        vec3 end   = grid_center + i * u_axis + grid_size * v_axis;
        // Render this line (from start to end)
      
        lines.push_back(vec3{start.x, start.y, start.z});
        lines.push_back(vec3{end.x, end.y, end.z});
    }

    return lines;
}
