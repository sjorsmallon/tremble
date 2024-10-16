#pragma once

#include "vec.hpp"

// while this is not 16 byte aligned, I do not care. if I need something more, I'll worry about it then. - SJM, 10-10-2024.
struct vertex_xnc
{
	vec3 position; // 12 bytes
	vec3 normal; // 12 bytes (24)
	vec4 color;  // 16 bytes (40)
}; 

static_assert(sizeof(vertex_xnc) == 40);


// Define formatter specialization for vec3 (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<vertex_xnc> : std::formatter<std::string> {
    // Format the vertex_xnc as a string
    auto format(const vertex_xnc& vertex, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "position: {}, normal: {},color: {})", vertex.position, vertex.normal, vertex.color);
    }
};


