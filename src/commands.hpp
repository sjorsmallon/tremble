#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <map>
#include <print>


inline std::vector<std::string> tokenize(const std::string& input)
{ 
    std::istringstream stream(input);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}


inline std::tuple<std::string, std::vector<std::string>> split_command(const std::string& input)
{
    auto tokens = tokenize(input);

    std::string command = tokens[0];
    tokens.erase(tokens.begin());  // Remove the command from the list of arguments

    return {command, tokens};  // Return the command and the arguments
}




struct Global_Variable
{
	std::string_view name;
};

template<typename Type>
Type get_value()
{
	
}

using Command = std::function<void(const std::vector<std::string>&)>;

struct Command_System
{
	std::map<std::string_view, Command> commands;
    std::map<std::string_view, Global_Variable> global_variables; 
};



// Note: I did not write this code. I yoinked it.
// the main story here is that I would want to support clients to write: 
// add_command(command_system, "command_name", pointer_to_the_function);
// where the function can be any function taking any arbitrary parameters (or uh, doubles, floats, ints and strings.).
// this means that when we provide the call with a vector of string (the tokenized input), we want to convert
// those values to the correct types that are described in the function signature.
// additionally, we want to check if we are not exceeding the number of parameters that the function actually takes.
// this template mess supports all that.

// Convert string to type T

// Convert string to type T
template <typename T>
T from_string(const std::string& str);

// Specializations for common types
template <>
int from_string<int>(const std::string& str) {
    return std::stoi(str);
}

template <>
float from_string<float>(const std::string& str) {
    return std::stof(str);
}

template <>
double from_string<double>(const std::string& str) {
    return std::stod(str);
}

template <>
std::string from_string<std::string>(const std::string& str) {
    return str;
}

// Helper function to invoke a function with converted arguments
template <typename Func, typename... Args, std::size_t... I>
void invoke_with_converted_args(Func&& func, const std::vector<std::string>& args, std::index_sequence<I...>)
{
    func(from_string<std::tuple_element_t<I, std::tuple<Args...>>>(args[I])...);
}


/// Register any command by passing the Command_System, the command name, and the function pointer
// template <typename Func, typename... Args>
// void register_command(Command_System& command_system, std::string_view name, Func&& func)
// {
//     command_system.commands[name] = [name, func = std::forward<Func>(func)](const std::vector<std::string>& args) {
//         if (args.size() != sizeof...(Args)) {  // Check if the number of arguments is correct
//             std::print("Error: Command '{}' expects {} arguments, but received {}.\n", name, sizeof...(Args), args.size());
//             return;
//         }

//         // Call the function with the converted arguments
//         invoke_with_converted_args(func, args, std::index_sequence_for<Args...>{});
//     };
// }

template <typename Func>
void register_command(Command_System& command_system, std::string_view name, Func&& func)
{
    // Use decltype to get the argument types of the function pointer
    using ArgTypes = typename function_traits<Func>::args;

    command_system.commands[name] = [name, func = std::forward<Func>(func)](const std::vector<std::string>& args) {
        if (args.size() != sizeof...(ArgTypes)) {  // Check if the number of arguments is correct
            std::cout << "Error: Command '" << name << "' expects " << sizeof...(ArgTypes) << " arguments, but received " << args.size() << ".\n";
            return;
        }

        // Call the function with the converted arguments
        invoke_with_converted_args(func, args, std::index_sequence_for<ArgTypes>{});
    };
}


// Function to execute a command by looking up the name in the Command_System
void execute_command(Command_System& command_system, std::string_view name, const std::vector<std::string>& args)
{
    auto it = command_system.commands.find(name);
    if (it != command_system.commands.end()) {
        // Execute the command
        it->second(args);
    } else {
        std::print("Error: Command '{}' not found.\n", name);
    }
}


// Function to execute a command by looking up the name in the Command_System
void execute_command(Command_System& command_system, std::string& command)
{
    //FIXME: this is not that nice. I want to use ranges but they are very slow.
    // just split the vector into the head and the rest.
    auto [command_name, command_arguments] = split_command(command); 



    auto it = command_system.commands.find(command_name);
    if (it != command_system.commands.end())
    {
        // Execute the command
        it->second(command_arguments);
    } else {
        std::print("Error: Command '{}' not found.\n", command_name);
    }
}