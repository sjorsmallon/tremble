#pragma once
#include <string>
#include <cstring> // for std::memcpy
// only do this for basic types
template <typename Type>
std::string to_byte_representation(Type& value)
{
	std::string destination{};
	destination.resize(sizeof(Type) + 1); // for null character?
	destination.back() = '\0';

	std::memcpy(destination[0], &value, sizeof(Type));
}