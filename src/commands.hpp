#pragma once

#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <map>
#include <print>

#include <functional>
#include <type_traits>
#include <typeinfo>


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


inline std::tuple<std::string, std::vector<std::string>> tokenize_and_split_command(const std::string& input)
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
std::function<void(const std::vector<std::string>&)> wrap_function_pointer_to_std_function_with_string_args(
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

template <typename Function>
void register_command(Command_System& command_system, std::string_view command_name, Function* function_ptr)
{
    auto std_function = wrap_function_pointer_to_std_function_with_string_args(function_ptr);
    command_system.commands[command_name] = std_function;
}


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



void execute_command(Command_System& command_system, std::string_view command_name, std::vector<std::string>& args)
{
    std::print("executing {}\n", command_name);
    std::print("with args: {}\n", args);
    if (!command_system.commands.contains(command_name))
    {
        std::print("command [{}] not found. returning.\n", command_name);
        return;
    }

    command_system.commands[command_name](args);
}