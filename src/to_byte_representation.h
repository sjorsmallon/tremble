#pragma once
#include <string>
#include <cstring> // for std::memcpy
#include <type_traits>


// only do this for basic types
template <Pod Pod_Type>
std::string to_byte_representation(Pod_Type& value)
{
	std::string result{};
	result.resize(sizeof(Pod_Type) + 1); // for null character?
	result.back() = '\0';

	std::memcpy(&result[0], &value, sizeof(Pod_Type));

	return result;
}