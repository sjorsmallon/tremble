#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>

// Helper: Split a string into tokens
std::vector<std::string> tokenize(const std::string& input)
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


struct Global_Variable
{
	std::string_view name;
}

template<typename Type>
Type get_value()
{
	
}


using Command = std::function<void(const std::vector<std::string>&)>;

struct Command_System
{
	std::map<std::string_view, Command> commands;
}
