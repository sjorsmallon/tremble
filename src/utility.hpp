#pragma once
#include "AABB.hpp"
#include <vector>
#include "vertex.hpp"


inline std::vector<vertex_xnc> to_vertices(AABB& aabb)
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


inline std::vector<vertex_xnc> to_vertices(std::vector<AABB>& aabbs)
{
	auto all_vertices = std::vector<vertex_xnc>{};
	for (auto& aabb: aabbs)
	{
		auto vertices = to_vertices(aabb);
		all_vertices.insert(all_vertices.end(), vertices.begin(), vertices.end());
	}

	return all_vertices;
}
