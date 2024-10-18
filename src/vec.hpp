#pragma once
#include <format>
#include <cmath>


#include <glm/glm.hpp>  
#include <glm/gtc/type_ptr.hpp>  // 

// Define formatter specialization for glm::vec3 (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<glm::vec3> : std::formatter<std::string> {
    // Format the vec3 as a string
    auto format(const glm::vec3& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};






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


    vec3 operator+(const vec3& other) const
    {
        return vec3{x + other.x, y + other.y, z + other.z};
    }

    vec3 operator-(const vec3& other) const
    {
        return vec3{x - other.x, y - other.y, z - other.z};
    }

    vec3& operator+=(const vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    // vec3 * float
    vec3 operator*(float scalar) const
    {
        return vec3{x * scalar, y * scalar, z * scalar};
    }

    // float * vec3 ("friend")
    friend vec3 operator*(float scalar, const vec3& v)
    {
        return vec3{v.x * scalar, v.y * scalar, v.z * scalar};
    }

};

// negate
vec3 operator-(const vec3& v)
{
    return vec3{-v.x, -v.y, -v.z};
}




inline vec3 normalize(vec3 v)
{
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);

    vec3 result = v;
    result.x /= length;
    result.y /= length;
    result.z /= length;

    return result;
}

inline float length(const vec3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline vec3 cross(const vec3& v1, const vec3& v2)
{
    return vec3{
        v1.y * v2.z - v1.z * v2.y,  // x component
        v1.z * v2.x - v1.x * v2.z,  // y component
        v1.x * v2.y - v1.y * v2.x   // z component
    };
}

// Free function: Dot product of two vectors
inline float dot(const vec3& v1, const vec3& v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// safeguard for some typecasting
static_assert(sizeof(vec3) == sizeof(glm::vec3));



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