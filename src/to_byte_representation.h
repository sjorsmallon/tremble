#pragma once
#include <string>
#include <cstring> // for std::memcpy
// only do this for basic types
template <typename Type>
std::string to_byte_representation(Type& value)
{
	std::string result{};
	result.resize(sizeof(Type) + 1); // for null character?
	result.back() = '\0';

	std::memcpy(&result[0], &value, sizeof(Type));

	return result;
}