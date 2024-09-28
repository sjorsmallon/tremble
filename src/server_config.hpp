#pragma once
#include <chrono>
using namespace std::chrono_literals;

namespace global
{
	static constexpr uint16_t port_number = 2020;
	static constexpr uint8_t iteration_count = 20;
	static constexpr auto iteration_duration = 1s;
}
