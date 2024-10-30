#pragma once
#include "vec.hpp"
#include <cfloat> // FLT_MAX
#include <cmath>
#include <functional>
// move to math 
size_t abs(size_t a, size_t b) {
    return (a > b) ? (a - b) : (b - a);
}

struct Plane
{
	vec3 point;
	vec3 normal;
};

Plane to_plane(vec3& v0, vec3& v1, vec3& v2)
{
	vec3 e0 =  normalize(v1 - v0);
	vec3 e1 =  normalize(v2 - v0);
	vec3 face_normal_at_v0 = normalize(cross(e0, e1));

	return Plane{.point = v0, .normal = face_normal_at_v0};
}

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
	uint64_t face_idx; // keep the face data in a contiguous array outside of the bsp, but refer to the "face_idx" so we know what we are talking about.
};

// partition result is "where do we put this face?"
Partition_Result get_partition_result(Plane& plane, const vec3& v0, const vec3& v1, const vec3& v2)
{
    float d0 = dot(plane.normal, v0 - plane.point);
    float d1 = dot(plane.normal, v1 - plane.point);
    float d2 = dot(plane.normal, v2 - plane.point);

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
        return Partition_Result::STRADDLING; // The face straddles the plane
    }
}

// move to math. Thank you, Christer Ericcson. I owe you one.
bool is_point_in_triangle(const vec3& point, const vec3& v0, const vec3& v1, const vec3& v2)
{
    // translate point and triangle so that point lies at origin
    vec3 a = v0 - point;
    vec3 b = v1 - point;
    vec3 c = v2 - point;

    // compute normal vectors for triangles pab and pbc.
    vec3 u = cross(b, c);
    vec3 v = cross(c, a);

    // make sure they are ponting in the same direction.
    if (dot(u, v) < 0.0f) return false;

    // compute normal vector for triangle pca
    vec3 w = cross(a, b);
    // make sure it points in the same direction as the first two
    if (dot(u, w) < 0.0f) return false;
    // otehrwise p must be in (or on) the triangle

    return true;
}


// move to math
float point_to_edge_distance(const vec3& point, const vec3& edge_start, const vec3& edge_end)
{
    vec3 edge = edge_end - edge_start;  // Vector along the edge
    vec3 point_to_start = point - edge_start;  // Vector from edge start to point

    // why the length squared?
    // Calculate the projection of the point onto the edge (normalized)
    float edge_length_squared = length_squared(edge);
    
    // Avoid division by zero if edge_length is zero
    if (edge_length_squared == 0.0f)
    {
        return distance_between(point, edge_start);  // Edge is a point, return distance to start
    }

    // Project the point onto the edge
    float t = dot(point_to_start, edge) / edge_length_squared;

    // Clamp the projection value t to the range [0, 1]
    t = std::max(0.0f, std::min(1.0f, t));

    // Find the closest point on the edge to the point
    vec3 closest_point = edge_start + edge * t;

    // Return the distance from the point to the closest point on the edge
    return distance_between(point, closest_point);
}


// move to math
// returns the distance to the closest point of any particular triangle.
float point_to_triangle_distance(const vec3& p, const vec3& v0, const vec3& v1, const vec3& v2)
{
    // get the normal of the triangle plane
    vec3 edge0 = v1 - v0;
    vec3 edge1 = v2 - v0;
    vec3 normal = cross(edge0, edge1);
    normal = normalize(normal);

    // Project the point onto the triangle's plane (draw a line from the point to perpendicularly to the triangle. find that collision.)
    vec3 v0_to_p = p - v0;
    float distance_to_plane = dot(v0_to_p, normal);

    // Step 3: Get the point projection on the plane
    vec3 projected_point = p - normal * distance_to_plane;

    // Step 4: Check if the projected point is inside the triangle
    bool inside = is_point_in_triangle(projected_point, v0, v1, v2);
    if (inside)
    {
        return fabs(distance_to_plane);  // If inside the triangle, this is the closest distance
    }

    // Step 5: Otherwise, calculate the minimum distance to the triangle's edges or vertices
    float dist_to_v0 = distance_between(p, v0);
    float dist_to_v1 = distance_between(p, v1);
    float dist_to_v2 = distance_between(p, v2);

    float dist_to_edge0 = point_to_edge_distance(p, v0, v1);
    float dist_to_edge1 = point_to_edge_distance(p, v1, v2);
    float dist_to_edge2 = point_to_edge_distance(p, v2, v0);

    return std::min({
        dist_to_v0,
        dist_to_v1,
        dist_to_v2,
        dist_to_edge0,
        dist_to_edge1,
        dist_to_edge2
    });
}

//@Memory: we never free this
//@Speed: Quadratic behavior! for all planes, for all other planes. or is it log?
// global_buffer is not const since it can be the case we add faces based on a STRADDLING face.
BSP* build_bsp(std::vector<uint64_t>& face_indices, std::vector<vertex_xnc>& all_faces_buffer)
{

	if (face_indices.empty())
	{
		return nullptr; // No faces to partition
    }

	//@Memory: yikes
	BSP* bsp = new BSP{};

	// base case: I have no idea yet. (why is the best_cadndiate unused? because it is just set automatically?)
	// uint64_t best_candidate = -1; 
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

		Plane plane = to_plane(v0, v1, v2);

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

			if (partition_result == Partition_Result::STRADDLING) // we should actually split I think.
			{
				front_indices.push_back(other_idx);
				back_indices.push_back(other_idx);
			}
		}

		// this is awful but whatever.
		if (initial) 
		{
			best_front_indices = std::move(front_indices);
			best_back_indices = std::move(back_indices);
			bsp->face_idx = candidate_idx;
			initial = false;
		}
		else
		{
			size_t absolute_delta = abs(front_indices.size(), back_indices.size());
			// smaller delta means more balanced tree.
			if (absolute_delta < abs(best_front_indices.size(), best_back_indices.size()))
			{
				best_front_indices = std::move(front_indices);
				best_back_indices = std::move(back_indices);
				bsp->face_idx = candidate_idx;
			}
		}

	}

	bsp->front = build_bsp(best_front_indices, all_faces_buffer);
	bsp->back = build_bsp(best_back_indices, all_faces_buffer);

	return bsp;
}


// the bsp should also have a pointer to the buffer it is based on, I guess?
// FIXME: cleaner would be std::pair<bool found, size_t index>.
inline size_t find_closest_proximity_face_index(BSP* bsp, const std::vector<vertex_xnc>& all_faces_buffer,const vec3& position, float distance_treshold)
{
	const size_t SENTINEL_FACE_IDX_NOT_FOUND = -1;

	if (bsp == nullptr)
	{
		return SENTINEL_FACE_IDX_NOT_FOUND;
	}

	auto& v0 = all_faces_buffer[bsp->face_idx].position;
	auto& v1 = all_faces_buffer[bsp->face_idx + 1].position;
	auto& v2 = all_faces_buffer[bsp->face_idx + 2].position;

	// how close are we actually? if we are in the plane described by the triangle, or even if we are not.
	float distance = point_to_triangle_distance(position, v0, v1, v2);

	// ok, this is it.
	if (distance < distance_treshold)
	{
		return bsp->face_idx;
	}

	BSP* first_branch = nullptr;
	BSP* second_branch = nullptr;

	// behind. start exhausting the back planes first.
	if (distance < 0.0f)
	{
		first_branch = bsp->back;
		second_branch = bsp->front;
	}
	else // in front. start exhausting the front planes first.
	{
		first_branch = bsp->front;
		second_branch = bsp->back;
	}

	size_t first_face_index = find_closest_proximity_face_index(first_branch, all_faces_buffer, position, distance_treshold);
	if (first_face_index != SENTINEL_FACE_IDX_NOT_FOUND)
	{
		return first_face_index;
	}

    // Check the second branch. 
    return find_closest_proximity_face_index(second_branch, all_faces_buffer, position, distance_treshold);
}






inline size_t find_closest_proximity_face_index_with_world_axis(BSP* bsp, const std::vector<vertex_xnc>& all_faces_buffer, const vec3& position, const vec3& world_axis, float distance_treshold)
{
    // any angle that is between 0 and 80 degrees from up is a floor.
    constexpr auto pm_floor_angle_treshold_deg = 80.f;

    const size_t SENTINEL_FACE_IDX_NOT_FOUND = -1;

    if (bsp == nullptr)
    {
        return SENTINEL_FACE_IDX_NOT_FOUND;
    }

    auto& v0 = all_faces_buffer[bsp->face_idx].position;
    auto& v1 = all_faces_buffer[bsp->face_idx + 1].position;
    auto& v2 = all_faces_buffer[bsp->face_idx + 2].position;

    // how close are we actually? if we are in the plane described by the triangle, or even if we are not.
    float distance = point_to_triangle_distance(position, v0, v1, v2);
    // ok, this is close enough.
    if (distance < distance_treshold)
    {
        vec3 e0 =  normalize(v1 - v0);
        vec3 e1 =  normalize(v2 - v0);
        vec3 face_normal_at_v0 = normalize(cross(e0, e1));

        //  calculate the angle w.r.t the world angle.
        auto dot_product=  dot(face_normal_at_v0, world_axis);
        float floor_angle = acos(dot_product);
        if (fabs(dot_product) < 0.000001f)//@FIXME: formalize this. if the up vector and the wall vector are orthogonal, the dot product is zero and we are just dealing with noise.
        {
            // do nothing.
        }
        else if (floor_angle < pm_floor_angle_treshold_deg && floor_angle > -pm_floor_angle_treshold_deg)
        {
            return bsp->face_idx;
        }
    }

    BSP* first_branch = nullptr;
    BSP* second_branch = nullptr;

    // behind. start exhausting the back planes first.
    if (distance < 0.0f)
    {
        first_branch = bsp->back;
        second_branch = bsp->front;
    }
    else // in front. start exhausting the front planes first.
    {
        first_branch = bsp->front;
        second_branch = bsp->back;
    }

    size_t first_face_index = find_closest_proximity_face_index_with_world_axis(first_branch, all_faces_buffer, position, world_axis, distance_treshold);
    if (first_face_index != SENTINEL_FACE_IDX_NOT_FOUND)
    {
        return first_face_index;
    }

    // Check the second branch. 
    return find_closest_proximity_face_index_with_world_axis(second_branch, all_faces_buffer, position, world_axis, distance_treshold);
}




inline vec3 compute_normal(const vec3& p0, const vec3& p1, const vec3& p2) {
    vec3 edge1 = p1 - p0;
    vec3 edge2 = p2 - p0;
    return normalize(cross(edge1, edge2));
}

// Free function to classify the position of the AABB relative to a plane
inline Partition_Result classify_aabb_against_plane(const AABB& aabb, const vec3& normal, const vec3& point)
{
    float d_min = dot(normal, aabb.min - point);
    float d_max = dot(normal, aabb.max - point);
    
    if (d_min > 0 && d_max > 0) {
        return Partition_Result::FRONT;  // Entirely in front
    } else if (d_min < 0 && d_max < 0) {
        return Partition_Result::BACK;   // Entirely behind
    } else {
        return Partition_Result::STRADDLING; // Straddling the plane
    }
}


inline std::vector<size_t> bsp_collide_with_AABB(BSP* bsp, const AABB& aabb, const std::vector<vertex_xnc>& all_faces_buffer) {
    
    std::vector<size_t> colliding_faces;

    // Recursive lambda for traversing the BSP tree
    std::function<void(BSP*)> traverse = [&](BSP* node) {
        if (node == nullptr) return;
        
        // Get the three vertices of the triangle from `all_faces_buffer`
        const vec3& p0 = all_faces_buffer[node->face_idx].position;
        const vec3& p1 = all_faces_buffer[node->face_idx + 1].position;
        const vec3& p2 = all_faces_buffer[node->face_idx + 2].position;
        
        // Compute the face normal
        vec3 normal = compute_normal(p0, p1, p2);
        
        // Classify the AABB's position relative to the current plane
        Partition_Result side = classify_aabb_against_plane(aabb, normal, p0);
        
        if (side == Partition_Result::FRONT || side == Partition_Result::STRADDLING) {
            traverse(node->front);
        }
        
        if (side == Partition_Result::BACK || side == Partition_Result::STRADDLING) {
            traverse(node->back);
        }
         else if (side == Partition_Result::STRADDLING) {
            // Traverse both front and back when straddling
            traverse(node->front);
            traverse(node->back);
            colliding_faces.push_back(node->face_idx);
        }
    };

    traverse(bsp);
    return colliding_faces;
}



















































    // BSP* first_branch = nullptr;
    // BSP* second_branch = nullptr;

    // if (distance >= 0.0f)
    // {
    //     // The point is on the front side of the plane
    //     first_branch = bsp->front;
    //     second_branch = bsp->back;
    // } else {
    //     // The point is on the back side of the plane
    //     first_branch = bsp->back;
    //     second_branch = bsp->front;
    // }


    // // Recursively search the first branch (closer to the position)
    // size_t closest_index_first = find_closest_proximity_face_index(first_branch, all_faces_buffer, position);

    // // Check the current BSP node's face as a potential candidate
    // float closest_distance = FLT_MAX;
    // size_t closest_face_index = -1; // sentinel value

    // if (closest_index_first != -1)
    // {
    //     // Calculate distance to the closest face in the first branch
    //     vec3 closest_point_on_face = get_closest_point_on_face(all_faces_buffer[closest_index_first], position);
    //     closest_distance = distance_between(position, closest_point_on_face);
    //     closest_face_index = closest_index_first;
    // }

    // // Check the current face's distance
    // vec3 closest_point_on_current_face = get_closest_point_on_face(v0, v1, v2, position);
    // float distance_to_current_face = distance_between(position, closest_point_on_current_face);

    // if (distance_to_current_face < closest_distance) {
    //     closest_face_index = bsp->index_of_face;
    //     closest_distance = distance_to_current_face;
    // }

    // // Recursively search the second branch (further from the position)
    // size_t closest_index_second = find_closest_proximity_face_index(second_branch, all_faces_buffer, position);

    // if (closest_index_second != -1) {
    //     vec3 closest_point_on_face = get_closest_point_on_face(all_faces_buffer[closest_index_second], position);
    //     float distance_to_second_face = distance_between(position, closest_point_on_face);

    //     if (distance_to_second_face < closest_distance) {
    //         closest_face_index = closest_index_second;
    //     }
    // }

    // return closest_face_index;
