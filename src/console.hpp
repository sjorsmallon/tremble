#pragma once
#include <print>
#include "concepts.hpp" // Ring_Buffer



struct Console
{
	Ring_Buffer<std::string> history = Ring_Buffer<std::string>(1000); // 1000 lines ought to be enough for anybody.
	std::string input_buffer = std::string(256, '\0');
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

void draw_line(const std::string_view line, const int start_x, const int start_y)
{
	std::println("render line:[{}]", line);
}

void draw_console(Console& console)
{
	constexpr auto height = 400;
	constexpr auto line_height = 20;
	constexpr auto margin = 5;
 	auto start_x = 10; // 
 	constexpr auto start_y = 10; // ARGH 2d conventions with Y pointing downwards. I don't want to commit to this.

	// render as many history lines as we can fit.
	const int lines_to_render = height / (line_height + margin); // character_height?
	// start at the top, render to the bottom.

    size_t history_size = console.history.size();
    size_t start_index = (console.history.full() ? console.history.capacity() : history_size) - lines_to_render;

    if (start_index < 0)
    {
        start_index += console.history.capacity();
    }

    // Render the history lines (this seems hilariously bad.)
    for (int idx = 0; idx < lines_to_render && history_size > 0; ++idx)
    {
        // Calculate the actual index to render from the ring buffer
        size_t index = (start_index + idx) % console.history.capacity();
        
        std::string line;
        if (console.history.get(index, line))
        {
        	// Calculate the current y position for rendering
            int current_y = start_y + idx * (line_height + margin);
        	draw_line(std::string_view{line}, start_x, current_y);
            start_x += 10;
        }
    }
}
