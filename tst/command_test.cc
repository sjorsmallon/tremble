#include "../src/commands.hpp"

#include <print>
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

	register_command(command_system, "example_function", &example_function);

	std::string new_command = "example_function 1 2.3";
	auto [command, args] = tokenize_and_split_command(new_command); 
	execute_command(command_system, command, args);

	// execute a command just from a string.
	execute_command_string(command_system, new_command);

	register_command(command_system, "hoi", &hello_anne_michele);
	execute_command(command_system, "hoi");

	// try to execute a command that does not exist.
	execute_command(command_system, "huh?");




	// bool noclip = false;
	// auto lambda = [&](){
	// 	noclip = !noclip;
	// };



	// auto result = wrap_lambda(lambda);

    return 0;
}