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

inline float length(const vec3& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


// messed up trick to avoid the "costly" sqrt.
inline float length_squared(const vec3& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}



inline float distance_between(const vec3& a, const vec3& b)
{
    vec3 diff = a - b;
    return length(diff);
}

// negate
vec3 operator-(const vec3& v)
{
    return vec3{-v.x, -v.y, -v.z};
}

// This normalize is giving me NaN issues in player_move. with normalizing the front vector. - Sjors, 22-10-2024
// inline vec3 normalize(vec3 v)
// {
//     float v_length = length(v);

//     vec3 result = v;
//     result.x /= v_length;
//     result.y /= v_length;
//     result.z /= v_length;

//     return result;
// }


inline vec3 normalize(vec3 v)
{
    float v_length = length(v);
    const float epsilon = 1e-8f;

    if (v_length > epsilon)
    {
        v.x /= v_length;
        v.y /= v_length;
        v.z /= v_length;
    }
    else
    {
        if (abs(v.x) > epsilon) //
        {
            v = vec3{(v.x > 0.0f ? 1.0f : -1.0f), 0.0f, 0.0f};
        }
        else if (abs(v.y) > epsilon)
        {
            v = vec3{0.0f, (v.y > 0.0f ? 1.0f : -1.0f), 0.0f};
        }
        else if (abs(v.z) > epsilon)
        {
            v = vec3{0.0f, 0.0f, (v.z > 0.0f ? 1.0f : -1.0f)};
        }
        else
        {
            v = vec3{0.0f, 0.0f, 0.0f};
        }
    }

    return v;
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