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


