#pragma once 
#include <vector>
#include <type_traits>

template <typename Type>
concept is_non_zero = (sizeof(Type) > 0);

// Define a POD concept using is_trivial and is_standard_layout
template<typename Type>
// concept Pod = std::is_trivial_v<T> && std::is_standard_layout_v<T>;
concept Pod = std::is_standard_layout_v<Type> && std::is_trivially_copyable_v<Type> && is_non_zero<Type>;



template<Pod Type>
class PodVector : public std::vector<Type> {
    // PodVector inherits all functionality of std::vector<T>
    // Add any additional functionality if required
public:
    using std::vector<Type>::vector; // Inherit constructors from std::vector
};

template <Pod Type, size_t N>
class Pod_Array : public std::array<Type, N> {

public:
    using std::array<Type, N>::array;
};

template<typename T>
concept PodVectorConcept = requires {
    typename T::value_type; // Check if T has a value_type
    requires Pod<typename T::value_type>; // Ensure value_type is Pod
};
