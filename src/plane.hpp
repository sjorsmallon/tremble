#pragma once
#include "vec.hpp"
#include <print>

struct Plane
{
	vec3 point;
	vec3 normal;
};

enum class Partition_Result
{
    BACK,
    FRONT,
    STRADDLING
};


Plane to_plane(vec3& v0, vec3& v1, vec3& v2)
{
	vec3 e0 =  normalize(v1 - v0);
	vec3 e1 =  normalize(v2 - v0);
	vec3 face_normal_at_v0 = normalize(cross(e0, e1));

	return Plane{.point = v0, .normal = face_normal_at_v0};
}


// because #derive[display, debug] is too fucking hard I guess.
template <>
struct std::formatter<Plane> : std::formatter<std::string> {
    // Format the vec3 as a string
    auto format(const Plane& plane, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "Plane: \n\tposition: [{}]\n\t normal: [{}]\n", plane.point, plane.normal);
    }
};
