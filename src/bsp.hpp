#pragma once
#include "vec.hpp"

// move to math 
size_t abs(size_t a, size_t b) {
    return (a > b) ? (a - b) : (b - a);
}

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

struct BSP
{
	BSP* front = nullptr;
	BSP* back = nullptr;
	uint64_t index_of_face; // keep the face data in a contiguous array outside of the bsp, but refer to the "face_idx" so we know what we are talking about.
};

Partition_Result get_partition_result(Plane& plane, const vec3& v0, const vec3& v1, const vec3& v2)
{
    float d0 = dot(plane.normal, v0 - plane.point);
    float d1 = dot(plane.normal, v1 - plane.point);
    float d2 = dot(plane.normal, v2 - plane.point);

    // std::print("d0, d1, d2: {}, {}, {}\n", d0, d1, d2);

    // check the signs of the distances
    bool is_v0_front = (d0 > 0.f);
    bool is_v1_front = (d1 > 0.f);
    bool is_v2_front = (d2 > 0.f);

    // all vertices are in front of the plane
    if (is_v0_front && is_v1_front && is_v2_front)
    {
        return Partition_Result::FRONT; 
    }
    // All vertices are behind the plane
    else if (!is_v0_front && !is_v1_front && !is_v2_front)
    {
        return Partition_Result::BACK; 
    }
    else
    {
        return Partition_Result::STRADDLING; // The triangle straddles the plane
    }
}



//@Speed: Quadratic behavior! for all planes, for all other planes. or is it log?
// global_buffer is not const since it can be the case we add faces based on a STRADDLING face.
BSP* build_bsp(std::vector<uint64_t>& face_indices, std::vector<vertex_xnc>& all_faces_buffer)
{
	//@Memory: yikes
	BSP* bsp = new BSP{};

	// base case: I have no idea yet.
	uint64_t best_candidate = -1; 
	std::vector<uint64_t> best_front_indices{};
	std::vector<uint64_t> best_back_indices{};
	bool initial = true;

	for (auto candidate_idx: face_indices)
	{
		std::vector<uint64_t> front_indices;
		std::vector<uint64_t> back_indices;

		// this can be a function but whatever
		vec3& v0 = all_faces_buffer[candidate_idx].position;
		vec3& v1 = all_faces_buffer[candidate_idx + 1].position;
		vec3& v2 = all_faces_buffer[candidate_idx + 2].position;

		auto e0 =  normalize(v1 - v0);
		auto e1 =  normalize(v2 - v0);
		auto face_normal_at_v0 = cross(e0, e1);

		Plane plane{.point = v0, .normal = face_normal_at_v0};

		for (auto other_idx: face_indices)
		{
			// skip myself
			if (other_idx == candidate_idx) continue;

			vec3& other_v0 = all_faces_buffer[other_idx].position;
			vec3& other_v1 = all_faces_buffer[other_idx + 1].position;
			vec3& other_v2 = all_faces_buffer[other_idx + 2].position;

			Partition_Result partition_result = get_partition_result(plane, other_v0, other_v1, other_v2);

			if (partition_result == Partition_Result::FRONT)
			{
				front_indices.push_back(other_idx);
			}
			if (partition_result == Partition_Result::BACK)
			{
				back_indices.push_back(other_idx);
			}

			if (partition_result == Partition_Result::STRADDLING)
			{
				front_indices.push_back(other_idx);
			}
		}

		// this is awful but whatever.
		if (initial) 
		{
			best_front_indices = front_indices;
			best_back_indices = back_indices;
			bsp->index_of_face = candidate_idx;
			best_candidate = candidate_idx;
			initial = false;
		}
		else
		{
			size_t absolute_delta = abs(front_indices.size(), back_indices.size());
			// smaller delta means more balanced tree.
			if (absolute_delta < abs(best_front_indices.size(), best_back_indices.size()))
			{
				std::print("found a new better dividing plane.\n");
				best_front_indices = front_indices;
				best_back_indices = back_indices;
				bsp->index_of_face = candidate_idx;
				best_candidate  = candidate_idx;
			}
		}

	}

	// BSP.front = build_bsp(front_indices, all_faces_buffer);
	// BSP.back = build_bsp(back_indices), all_Faces_buffer;

	assert(best_front_indices.size() > 0);
	assert(best_back_indices.size() > 0);

	
	// for debug purposes: color front facing green, color back facing red.
	for (auto index: best_front_indices)
	{
		auto& v0 = all_faces_buffer[index];
		auto& v1 = all_faces_buffer[index + 1];
		auto& v2 = all_faces_buffer[index + 2];

		// teal
		v0.color = vec4{0.0f, 1.0f, 0.0f,1.0f};
		v1.color = vec4{0.0f, 1.0f, 0.0f,1.0f};
		v2.color = vec4{0.0f, 1.0f, 0.0f,1.0f};
	}

	for (auto index: best_back_indices)
	{
		auto& v0 = all_faces_buffer[index];
		auto& v1 = all_faces_buffer[index + 1];
		auto& v2 = all_faces_buffer[index + 2];

		//red
		v0.color = vec4{1.0f, 0.0f, 0.0f,1.0f};
		v1.color = vec4{1.0f, 0.0f, 0.0f,1.0f};
		v2.color = vec4{1.0f, 0.0f, 0.0f,1.0f};
	}

	// the dividing plane.

	auto& v0 = all_faces_buffer[best_candidate];
	auto& v1 = all_faces_buffer[best_candidate + 1];
	auto& v2 = all_faces_buffer[best_candidate + 2];

	//white
	v0.color = vec4{1.0f, 1.0f, 1.0f,1.0f};
	v1.color = vec4{1.0f, 1.0f, 1.0f,1.0f};
	v2.color = vec4{1.0f, 1.0f, 1.0f,1.0f};

	return bsp;
}




