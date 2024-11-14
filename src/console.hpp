#pragma once
#include <print>
#include "concepts.hpp" // Ring_Buffer

struct Console
{
	Ring_Buffer<std::string> history = Ring_Buffer<std::string>(1000); // 1000 lines ought to be enough for anybody.
	std::string input_buffer; // FIXME: preallocate?
};

void handle_keystroke(Console& console, char key)
{
    if (key == '\n')
    {
        console.history.push(console.input_buffer);

        console.input_buffer.clear();
    } else if (key == '\b')
    {
        if (!console.input_buffer.empty())
        {
            console.input_buffer.pop_back();
	    }
    } else {
        // Append the character to the input buffer
        console.input_buffer += key;
    }
}

