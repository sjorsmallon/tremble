#pragma once
#include "vec.hpp"
#include <format>

struct AABB
{
	vec3 min;
	vec3 max;
};

// Define formatter specialization for AABB (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<AABB> : std::formatter<std::string> {
    // Format the AABB as a string
    auto format(const AABB& aabb, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "AABB(min: {}, max: {})", aabb.min, aabb.max);
    }
};


// some code to parse a file and return a vector of AABB.
// file should look like: 
// min: a b c max: x y z

#include <print>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

inline std::vector<AABB> read_AABBs_from_file(std::string& filename)
{
	auto parse_line_to_aabb = [](const std::string& line) -> AABB {
		
		auto aabb = AABB{};
	    std::stringstream ss(line);
	    std::string temp;

	    // Parse the 'min:' part
	    ss >> temp >> aabb.min.x >> aabb.min.y >> aabb.min.z;

	    // Parse the 'max:' part
	    ss >> temp >> aabb.max.x >> aabb.max.y >> aabb.max.z;

	    return aabb;
	};

	auto aabbs = std::vector<AABB>{};
	auto file = std::ifstream{filename};

    if (!file.is_open())
    {
        std::print("Failed to open file: {}\n", filename);
        return aabbs;
    }

    auto line = std::string{};
    while (std::getline(file, line))
    {
        if (line.empty()) continue;  // Skip empty lines
        aabbs.push_back(parse_line_to_aabb(line));
    }

    return aabbs;
}


#include "vertex.hpp"

// I do not know where to put these, but to create a whole new file was confusing me 1 sessions later.
inline std::vector<vertex_xnc> to_vertex_xnc(AABB& aabb)
{
	auto vertices = std::vector<vertex_xnc>{
        vertex_xnc{.position = vec3{aabb.min.x, aabb.min.y, aabb.min.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {1.f, 0.f, 0.f}}, // 0
        vertex_xnc{.position = vec3{aabb.max.x, aabb.min.y, aabb.min.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {0.f, 1.f, 0.f}}, // 1
        vertex_xnc{.position = vec3{aabb.max.x, aabb.max.y, aabb.min.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {0.f, 0.f, 1.f}}, // 2
        vertex_xnc{.position = vec3{aabb.min.x, aabb.max.y, aabb.min.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {1.f, 0.f, 0.f}}, // 3
        vertex_xnc{.position = vec3{aabb.min.x, aabb.min.y, aabb.max.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {0.f, 1.f, 0.f}}, // 4
        vertex_xnc{.position = vec3{aabb.max.x, aabb.min.y, aabb.max.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {0.f, 0.f, 1.f}}, // 5
        vertex_xnc{.position = vec3{aabb.max.x, aabb.max.y, aabb.max.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {1.f, 0.f, 0.f}}, // 6
        vertex_xnc{.position = vec3{aabb.min.x, aabb.max.y, aabb.max.z}, .normal = vec3{1.0f, 0.0f, 0.0f}, .color = {0.f, 1.f, 0.f}}  // 7
    };

    auto indices = std::vector<uint32_t>{
        0, 1, 2, 0, 2, 3, // Front face
        4, 5, 6, 4, 6, 7, // Back face
        0, 1, 5, 0, 5, 4, // Bottom face
        3, 2, 6, 3, 6, 7, // Top face
        0, 3, 7, 0, 7, 4, // Left face
        1, 2, 6, 1, 6, 5  // Right face
    };


    // okay, for now, disregard the index buffer. let us just construct the full buffer here.
    auto all_vertices = std::vector<vertex_xnc>{
        vertices[0], vertices[1], vertices[2], vertices[0], vertices[2], vertices[3], // Front face
        vertices[4], vertices[5], vertices[6], vertices[4], vertices[6], vertices[7], // Back face
        vertices[0], vertices[1], vertices[5], vertices[0], vertices[5], vertices[4], // Bottom face
        vertices[3], vertices[2], vertices[6], vertices[3], vertices[6], vertices[7], // Top face
        vertices[0], vertices[3], vertices[7], vertices[0], vertices[7], vertices[4], // Left face
        vertices[1], vertices[2], vertices[6], vertices[1], vertices[6], vertices[5]  // Right face
    };


    return all_vertices;
}

inline std::vector<vertex_xnc> to_vertex_xnc(std::vector<AABB>& aabbs)
{
	auto all_vertices = std::vector<vertex_xnc>{};
	for (auto& aabb: aabbs)
	{
		auto vertices = to_vertex_xnc(aabb);
		all_vertices.insert(all_vertices.end(), vertices.begin(), vertices.end());
	}

	return all_vertices;
}
