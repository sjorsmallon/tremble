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


// Sample function to demonstrate the wrapping
int example_function(int x, double y) {
    std::cout << "Function called with x = " << x << " and y = " << y << std::endl;
    return x + static_cast<int>(y);  // Adds the integer and casted double
}

void whatever()
{
	std::print("whatever.\n");
}

void hello()
{
	std::print("Hello, Sailor!\n");
}

void hello_anne_michele()
{
	std::print("hello Anne-Michèle!\n");
}


int main() {
	Command_System command_system{};
	register_command(command_system, "hello", &hello);
	std::vector<std::string> no_args{};
	command_system.commands["hello"](no_args);

	register_command(command_system, "example_function", &example_function);

	std::string new_command = "example_function 1 2.3";
	auto [command, args] = tokenize_and_split_command(new_command); 

	execute_command(command_system, command, args);

	register_command(command_system, "hoi", &hello_anne_michele);


	execute_command(command_system, "hoi", no_args);


    return 0;
}