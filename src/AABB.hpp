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