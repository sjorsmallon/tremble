#pragma once
#include "vec.hpp"
#include <format>
#include <random>

struct AABB
{
	vec3 min;
	vec3 max;
};

// Define formatter specialization for AABB (THE CONST AFTER THE MEMBER FN IS NEEDED BECAUSE OTHERWISE YOU GET 14000000 LINES OF TEMPLATE ERRORS)
template <>
struct std::formatter<AABB> : std::formatter<std::string>
{
    // Format the AABB as a string
    auto format(const AABB& aabb, std::format_context& ctx) const
    {
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

std::vector<std::string> split_string(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);  
    std::string token;

    
    while (std::getline(ss, token, delimiter))
    {
        if (!token.empty()) 
        {  
            tokens.push_back(token);
        }
    }
    return tokens;
}


inline std::vector<AABB> read_AABBs_from_file(std::string& filename)
{
	auto parse_line_to_aabb = [](const std::string& line) -> AABB{
		auto aabb = AABB{};
	    std::stringstream ss(line);
	    std::string temp;

	    if (line.find("{") != std::string::npos)
        {
            // Format: min: {-2000, -2000, -2000}, max: {2000, -1000, 2000}
            std::stringstream ss(line);
            std::string token;
            // Parse "min" values
            std::getline(ss, token, '{'); // Discard "min: {"
            std::getline(ss, token, '}');
            auto minValues = split_string(token, ',');
            aabb.min = vec3{std::stof(minValues[0]), std::stof(minValues[1]), std::stof(minValues[2])};

            // Parse "max" values
            std::getline(ss, token, '{'); // Discard ", max: {"
            std::getline(ss, token, '}');
            auto maxValues = split_string(token, ',');
            aabb.max = vec3{std::stof(maxValues[0]), std::stof(maxValues[1]), std::stof(maxValues[2])};
        }
        std::print("parsed aabb. min: {}, max: {}\n", aabb.min, aabb.max);
	    return aabb;
	};

	auto aabbs = std::vector<AABB>{};
	auto file = std::ifstream(filename);

    if (!file.is_open())
    {
        std::print("Failed to open file: {}\n", filename);
        return aabbs;
    }

    auto line = std::string{};
    while (std::getline(file, line))
    {
        if (line.empty()) continue;  
        if (line.starts_with('#')) continue;
        aabbs.push_back(parse_line_to_aabb(line));
    }

    return aabbs;
}


bool are_overlapping(const AABB& a, const AABB& b)
{
    return (a.min.x < b.max.x && a.max.x > b.min.x &&
            a.min.y < b.max.y && a.max.y > b.min.y &&
            a.min.z < b.max.z && a.max.z > b.min.z);
}

std::vector<AABB> generate_non_overlapping_aabbs(int num_aabbs, const vec3& extents, const AABB& bounds)
{
    std::vector<AABB> aabbs;
    std::random_device rd; // Obtain a random number from hardware
    std::mt19937 eng(rd()); // Seed the generator

    std::uniform_real_distribution<> distr_x(bounds.min.x + extents.x / 2, bounds.max.x - extents.x / 2);
    std::uniform_real_distribution<> distr_y(bounds.min.y + extents.y / 2, bounds.max.y - extents.y / 2);
    std::uniform_real_distribution<> distr_z(bounds.min.z + extents.z / 2, bounds.max.z - extents.z / 2);

    while (aabbs.size() < num_aabbs) {
        // Generate random center for the AABB
        float center_x = distr_x(eng);
        float center_y = distr_y(eng);
        float center_z = distr_z(eng);

        // Calculate the min and max points of the AABB
        vec3 aabb_min{center_x - extents.x / 2, center_y - extents.y / 2, center_z - extents.z / 2};
        vec3 aabb_max{center_x + extents.x / 2, center_y + extents.y / 2, center_z + extents.z / 2};

        // Create the AABB
        AABB new_aabb(aabb_min, aabb_max);

        // Check for overlaps with existing AABBs
        bool overlap = false;
        for (const auto& existing_aabb : aabbs)
        {
            if (are_overlapping(new_aabb, existing_aabb))
            {
                overlap = true;
                break;
            }
        }

        // If no overlap, add the new AABB to the list
        if (!overlap) {
            aabbs.push_back(new_aabb);
        }
    }

    return aabbs;
}


#include "vertex.hpp"

// I do not know where to put these, but to create a whole new file was confusing me 1 sessions later.
inline std::vector<vertex_xnc> to_vertex_xnc(AABB& aabb)
{
	// clang does not like initialization of the union vec3 like this. it starts complaining about missing brackets.
	// but I think this should be fine.
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-braces"

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


    // this has the top and bottom faces generated in the "wrong" order for back-face culling.
    // auto indices = std::vector<uint32_t>{
    //     0, 1, 2, 0, 2, 3, // Front face
    //     4, 5, 6, 4, 6, 7, // Back face
    //     0, 1, 5, 0, 5, 4, // Bottom face
    //     3, 2, 6, 3, 6, 7, // Top face
    //     0, 3, 7, 0, 7, 4, // Left face
    //     1, 2, 6, 1, 6, 5  // Right face
    // };

    auto indices = std::vector<uint32_t>{
        0, 1, 2, 0, 2, 3, // Front face
        4, 5, 6, 4, 6, 7, // Back face
        5, 1, 0, 4, 5, 0, // Bottom face
        6, 2, 3, 7, 6, 3, // Top face
        0, 3, 7, 0, 7, 4, // Left face
        1, 2, 6, 1, 6, 5  // Right face
    };


    // okay, for now, disregard the index buffer. let us just construct the full buffer here.
    // auto all_vertices = std::vector<vertex_xnc>{
    //     vertices[0], vertices[1], vertices[2], vertices[0], vertices[2], vertices[3], // Front face
    //     vertices[4], vertices[5], vertices[6], vertices[4], vertices[6], vertices[7], // Back face
    //     vertices[5], vertices[1], vertices[0], vertices[4], vertices[5], vertices[0], // Bottom face
    //     vertices[6], vertices[2], vertices[3], vertices[7], vertices[6], vertices[3], // Top face
    //     vertices[0], vertices[3], vertices[7], vertices[0], vertices[7], vertices[4], // Left face
    //     vertices[1], vertices[2], vertices[6], vertices[1], vertices[6], vertices[5]  // Right face
    // };

    // Define the colors for each face (RGB)
    // vec3 front_color {1.0f, 0.0f, 0.0f};  // Red
    // vec3 back_color  {0.0f, 1.0f, 0.0f};  // Green
    // vec3 bottom_color{0.0f, 0.0f, 1.0f};  // Blue
    // vec3 top_color   {1.0f, 1.0f, 0.0f};  // Yellow
    // vec3 left_color  {1.0f, 0.0f, 1.0f};  // Magenta
    // vec3 right_color {0.0f, 1.0f, 1.0f};  // Cyan

    // Assign the color to each face's vertices by index. colors are hardcoded like this vbecause the compiler was complaining.
    auto all_vertices = std::vector<vertex_xnc>{
    // Front face 
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal, .color =  {0.8f, 0.0f, 0.0f}},
    vertex_xnc{.position = vertices[2].position, .normal =  vertices[2].normal, .color =  {0.8f, 0.0f, 0.0f}}, 
    vertex_xnc{.position = vertices[1].position, .normal =  vertices[1].normal, .color =  {0.8f, 0.0f, 0.0f}}, 
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal, .color =  {0.8f, 0.0f, 0.0f}},
    vertex_xnc{.position = vertices[3].position, .normal =  vertices[3].normal, .color =  {0.8f, 0.0f, 0.0f}},
    vertex_xnc{.position = vertices[2].position, .normal =  vertices[2].normal, .color =  {0.8f, 0.0f, 0.0f}}, 

    // Back face
    vertex_xnc{.position = vertices[4].position, .normal =  vertices[4].normal, .color =   {0.0f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[5].position, .normal =  vertices[5].normal, .color =   {0.0f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal, .color =   {0.0f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[4].position, .normal =  vertices[4].normal, .color =   {0.0f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal, .color =   {0.0f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[7].position, .normal =  vertices[7].normal, .color =   {0.0f, 0.75f, 0.0f}},

    // Bottom face 
    vertex_xnc{.position = vertices[5].position, .normal =  vertices[5].normal,.color =  {0.0f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal,.color =  {0.0f, 0.0f, 0.75f}}, 
    vertex_xnc{.position = vertices[1].position, .normal =  vertices[1].normal,.color =  {0.0f, 0.0f, 0.75f}}, 
    vertex_xnc{.position = vertices[4].position, .normal =  vertices[4].normal,.color =  {0.0f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal,.color =  {0.0f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[5].position, .normal =  vertices[5].normal,.color =  {0.0f, 0.0f, 0.75f}},

    // Top face
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal,.color =  {0.75f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[2].position, .normal =  vertices[2].normal,.color =  {0.75f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[3].position, .normal =  vertices[3].normal,.color =  {0.75f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[7].position, .normal =  vertices[7].normal,.color =  {0.75f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal,.color =  {0.75f, 0.75f, 0.0f}},
    vertex_xnc{.position = vertices[3].position, .normal =  vertices[3].normal,.color =  {0.75f, 0.75f, 0.0f}},

    // Left face 
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal,.color =  {0.75f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[7].position, .normal =  vertices[7].normal,.color =  {0.75f, 0.0f, 0.75f}}, 
    vertex_xnc{.position = vertices[3].position, .normal =  vertices[3].normal,.color =  {0.75f, 0.0f, 0.75f}}, 
    vertex_xnc{.position = vertices[0].position, .normal =  vertices[0].normal,.color =  {0.75f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[4].position, .normal =  vertices[4].normal,.color =  {0.75f, 0.0f, 0.75f}},
    vertex_xnc{.position = vertices[7].position, .normal =  vertices[7].normal,.color =  {0.75f, 0.0f, 0.75f}}, 

    // Right face
    vertex_xnc{.position = vertices[1].position, .normal =  vertices[1].normal,.color =  {0.0f, 0.75f, 0.75f}},
    vertex_xnc{.position = vertices[2].position, .normal =  vertices[2].normal,.color =  {0.0f, 0.75f, 0.75f}},
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal,.color =  {0.0f, 0.75f, 0.75f}},
    vertex_xnc{.position = vertices[1].position, .normal =  vertices[1].normal,.color =  {0.0f, 0.75f, 0.75f}},
    vertex_xnc{.position = vertices[6].position, .normal =  vertices[6].normal,.color =  {0.0f, 0.75f, 0.75f}},
    vertex_xnc{.position = vertices[5].position, .normal =  vertices[5].normal,.color =  {0.0f, 0.75f, 0.75f}}
    }; 

    #pragma clang diagnostic pop


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
