#pragma once
#include <format>

struct vec4
{
    union
    {
        struct
        {
            float x, y, z, w;  // For 3D coordinates
        };
        
        struct
        {
            float r, g, b, a;  // For RGB colors
        };
        struct
        {
            float data[4];
        };
    };
};

// Define formatter specialization for vec3 (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<vec4> : std::formatter<std::string> {
    // Format the vec3 as a string
    auto format(const vec4& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", v.x, v.y, v.z, v.w);
    }
};


struct vec3
{
    union
    {
        struct
        {
            float x, y, z;  // For 3D coordinates
        };
        
        struct
        {
            float r, g, b;  // For RGB colors
        };
        struct
        {
        	float data[3];
        };
    };
};

// Define formatter specialization for vec3 (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<vec3> : std::formatter<std::string> {
    // Format the vec3 as a string
    auto format(const vec3& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};



struct vec2
{
    union
    {
        struct
        {
            float x, y;  // For 3D coordinates
        };
        
        struct
        {
            float u, v;  // For RGB colors
        };
        struct
        {
            float data[2];
        };
    };
};

// Define formatter specialization for vec2 (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<vec2> : std::formatter<std::string> {
    // Format the vec3 as a string
    auto format(const vec2& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};