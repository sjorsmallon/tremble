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

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) 
        : m_buffer(capacity), m_head(0), m_size(0), m_capacity(capacity) {}

    void push(const T& item) {
        m_buffer[m_head] = item; // Overwrite the oldest item
        m_head = (m_head + 1) % m_capacity; // Move head forward
        if (m_size < m_capacity) {
            ++m_size; // Increase size until we hit capacity
        }
    }

    bool full() const {
        return m_size == m_capacity;
    }

    size_t size() const {
        return m_size;
    }

    size_t capacity() const {
        return m_capacity;
    }

    // Check if the buffer is empty
    bool empty() const {
        return m_size == 0;
    }

    // Get an item at a specific index without removing it
    bool get(size_t index, T& item) const {
        if (index >= m_size) {
            return false; // Out of bounds
        }
        item = m_buffer[(m_head + index) % m_capacity];
        return true; // Successfully retrieved the item
    }

    // Method to get the latest added item by reference (back of the buffer)
    T& back() {
        return m_buffer[(m_head + m_capacity - 1) % m_capacity]; // Return a reference to the last item added
    }

    // Const version for read-only access
    const T& back() const {
        return m_buffer[(m_head + m_capacity - 1) % m_capacity]; // Return a const reference to the last item added
    }

private:
    std::vector<T> m_buffer; // The underlying storage for the ring buffer
    size_t m_head;           // Index of the next insertion point
    size_t m_size;           // Current number of items in the buffer
    size_t m_capacity;       // Maximum capacity of the buffer
};
