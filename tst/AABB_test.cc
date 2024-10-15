#include "../src/AABB.hpp"
#include <string>


int main()
{
	auto path = std::string{"data/list_of_AABB"};
	auto aabbs = read_AABBs_from_file(path);

	for (auto& aabb: aabbs)
	{
		std::print("{}\n", aabb);
	}

	auto vertices = to_vertex_xnc(aabbs);
	for (auto& vertex: vertices)
	{
		std::print("{}\n", vertex);
	}

}