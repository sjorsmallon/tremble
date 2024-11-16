#include "../src/commands.hpp"
#include <print>

static void announce(const std::string_view)
{
	std::print("announce!\n");
}

static void set_some_values(int a, int b, float c)
{
	std::print("set_some_values: {}, {} {}\n", a, b, c);
}

int main()
{
	Command_System command_system{};
	register_command(command_system, "set_some_values", &set_some_values);
	std::string some_values_command = "set_some_values 0 1 2";
	execute_command(command_system, some_values_command);

}