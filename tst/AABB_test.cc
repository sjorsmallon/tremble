#include "../src/AABB.hpp"
#include "../src/utility.hpp" // for to_vertices(AABBs).
#include <string>


int main()
{
	auto path = std::string{"data/list_of_AABB"};
	auto aabbs = read_AABBs_from_file(path);

	for (auto& aabb: aabbs)
	{
		std::print("{}\n", aabb);
	}

	auto vertices = to_vertices(aabbs);
	for (auto& vertex: vertices)
	{
		std::print("{}\n", vertex);
	}

}