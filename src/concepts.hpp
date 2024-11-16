#pragma once 
#include <vector>
#include <type_traits>
#include <unordered_set>
#include <print>

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
std::vector<T> concatenate(const std::vector<T>& vec1, const std::vector<T>& vec2) {
    std::vector<T> result;
    result.reserve(vec1.size() + vec2.size()); // Preallocate memory for efficiency

    // Insert elements from both vec1 and vec2
    result.insert(result.end(), vec1.begin(), vec1.end());
    result.insert(result.end(), vec2.begin(), vec2.end());

    return result;
}

template <typename Type>
std::vector<Type> filter_duplicates(const std::vector<Type>& vec)
{
    std::unordered_set<Type> seen;
    std::vector<Type> unique_elements;

    for (size_t value : vec) {
        if (seen.find(value) == seen.end()) {
            seen.insert(value); 
            unique_elements.push_back(value);
        }
    }
    return unique_elements;
};



template <typename T>
class Ring_Buffer
{
public:
    explicit Ring_Buffer(size_t capacity) 
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
        if (index >= m_size)
        {
            return false; // Out of bounds
        }
        item = m_buffer[index];
        return true; // Successfully retrieved the item
    }

    // Method to get the latest added item by reference (back of the buffer)
    T& back() {
        return m_buffer[(m_head + m_capacity - 1) % m_capacity]; // Return a reference to the last item added
    }

    size_t index_of_latest_entry()
    {
        return (m_head + m_capacity - 1) % m_capacity;
    }


    size_t wrap_index(size_t index) const
    {
        return (index + m_capacity) % m_capacity;
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


#define TIMEIT(stmt) do { \
    auto start = std::chrono::high_resolution_clock::now(); \
    stmt; \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start); \
    std::print("{} took {:.3f} ms\n", #stmt, diff.count() / 1000.0); \
} while(0)








//@FIXME: move all these formatters to some other place.
template <>
struct std::formatter<std::vector<std::string>> : std::formatter<std::string> {
    // Format the vector as a comma-separated list of quoted strings
    auto format(const std::vector<std::string>& vec, std::format_context& ctx) const {
        auto out = ctx.out();
        bool first = true;
        
        // Start the format with an opening bracket
        out = std::format_to(out, "[");
        
        // Iterate through the vector of strings
        for (const auto& value : vec) {
            if (!first) {
                // Add a comma separator between elements
                out = std::format_to(out, ", ");
            }
            // Format each string as quoted, e.g., "apple"
            out = std::format_to(out, "\"{}\"", value);
            first = false;
        }
        
        // End the format with a closing bracket
        return std::format_to(out, "]");
    }
};

namespace console_colors
{
    constexpr const char* reset = "\033[0m";
    constexpr const char* yellow = "\033[33m";
}

template <typename... Args>
void print_warning(std::string_view format_str, Args&&... args) {
    // Create the full formatted string with color codes
    std::string formatted_message = std::format("{}[warning] {}{}", console_colors::yellow,
                                                std::vformat(format_str, std::make_format_args(std::forward<Args>(args)...)),
                                                console_colors::reset);

    // Print the formatted message
    std::print("{}\n", formatted_message);
}



