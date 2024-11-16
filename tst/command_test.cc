#include "../src/commands.hpp"
#include <print>

// // A sample function to test.
// int example_function(int x) {
//     return x * 2;
// }

// void void_example_function(int x)
// {
// 	std::print("x: {}\n", x);
// }

// // A generic function that converts a function pointer to a std::function.
// template <typename Func>
// std::function<Func> convert_to_std_function(Func* func_ptr) {
//     return std::function<Func>(func_ptr);  // Wrap the function pointer into std::function
// }


// template <typename Func>
// std::function<void(std::vector<std::string>)> wrap_function(Func* func_ptr) {
//     return [func_ptr](std::vector<std::string> args) {
//         // Convert the first string in the vector to an integer (assuming the function takes int)
//         if (args.size() != 1) {
//             std::print("Expected exactly one argument!");
//             return;
//         }

//         int arg = std::stoi(args[0]);  // Convert string to integer
//         int result = (*func_ptr)(arg);  // Call the original function with the argument
//         std::cout << "Result: " << result << std::endl;  // Output the result
//     };
// }



// int main() {
//     // Use the function pointer to create a std::function
//     auto func = convert_to_std_function(example_function);
//     auto void_func = convert_to_std_function(void_example_function);

//     return 0;
// }
#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

// Helper function to convert string to various types
template <typename T>
T convert_from_string(const std::string& str) {
    std::istringstream stream(str);
    T value;
    stream >> value;
    if (stream.fail()) {
        throw std::invalid_argument("Conversion failed for string: " + str);
    }
    return value;
}


// Helper function to convert each argument from string to the correct type
template <std::size_t Index = 0, typename... Args>
void convert_args(const std::vector<std::string>& args, std::tuple<Args...>& tuple) {
    if constexpr (Index < sizeof...(Args)) {
        // Convert the argument at the current index
        std::get<Index>(tuple) = convert_from_string<std::tuple_element_t<Index, std::tuple<Args...>>>(args[Index]);
        // Recursively convert the next argument
        convert_args<Index + 1>(args, tuple);
    }
}

// A wrapper function that can wrap a function with any signature that takes arguments
// from a vector of strings and returns void.
template <typename Ret, typename... Args>
std::function<void(std::vector<std::string>)> wrap_function(std::function<Ret(Args...)> func) {
    return [func](std::vector<std::string> args) {
        if (args.size() != sizeof...(Args)) {
            std::print("Incorrect number of arguments!\n");
            return;
        }

        // Create a tuple of converted arguments
        std::tuple<Args...> converted_args;
        try {
            // Convert each string argument to the correct type and store it in the tuple
            convert_args<0>(args, converted_args);
        } catch (const std::exception& e) {
            std::print("Error converting arguments: {}\n", e.what()); 
            return;
        }

        // Call the original function with unpacked arguments
        std::apply(func, converted_args);
    };
}




// Helper function to convert a function pointer to a std::function
template <typename Ret, typename... Args>
std::function<Ret(Args...)> function_pointer_to_std_function(Ret (*func)(Args...)) {
    return [func](Args... args) { return func(args...); };
}


// A function that takes a function pointer and returns a std::function that
// accepts a vector of strings, converts the strings to arguments, and calls the function
template <typename Ret, typename... Args>
std::function<void(std::vector<std::string>&)> wrap_function_pointer_to_std_function_with_string_args(
    Ret (*func)(Args...)
) {
    return [func](std::vector<std::string> args) {
        if (args.size() != sizeof...(Args)) {
            std::print("Incorrect number of arguments!\n");
            return;
        }

        // Create a tuple of converted arguments
        std::tuple<Args...> converted_args;
        try {
            // Convert each string argument to the correct type and store it in the tuple
            convert_args<0>(args, converted_args);
        } catch (const std::exception& e) {
            std::print("Error converting arguments: {} \n", e.what());
            return;
        }

        // Call the original function with unpacked arguments
        std::apply(func, converted_args);
    };
}


// Sample function to demonstrate the wrapping
int example_function(int x, double y) {
    std::cout << "Function called with x = " << x << " and y = " << y << std::endl;
    return x + static_cast<int>(y);  // Adds the integer and casted double
}

void whatever()
{
	std::print("whatever.\n");
}


int main() {
	std::vector<std::function<void(std::vector<std::string>&)>> wrapped_functions;

    // Wrap the example function into a std::function that accepts a vector of strings
	auto wrapped_function_1 = wrap_function_pointer_to_std_function_with_string_args(&example_function);
	auto wrapped_function_2 = wrap_function_pointer_to_std_function_with_string_args(&whatever);

	wrapped_functions.push_back(wrapped_function_1);
	wrapped_functions.push_back(wrapped_function_2);

    std::vector<std::string> args = {"5", "3.14"};
    wrapped_functions[0](args);  // Should print "Function called with x = 5 and y = 3.14"
    wrapped_functions[1](args);  // Should print "Function called with x = 5 and y = 3.14"

    auto no_args = std::vector<std::string>{};
    wrapped_functions[1](no_args);  // Should print "Function called with x = 5 and y = 3.14"






    return 0;
}