#pragma once
#include <print>
#include "concepts.hpp" // Ring_Buffer
#include "keys.hpp" // keycode

struct Console
{
	Ring_Buffer<std::string> history = Ring_Buffer<std::string>(1000); // 1000 lines ought to be enough for anybody.
	std::string input_buffer; // FIXME: preallocate?
    size_t caret_idx; // place the caret before this character.
    bool currently_scrolling_through_history_via_up_arrow;
    size_t index_of_history_entry_that_is_currently_displayed;
};


inline void clear_input(Console& console)
{
    console.input_buffer.clear();
}

inline void handle_keystroke(Console& console, Keys::Keycode key, bool shift_pressed, bool control_pressed)
{
    if (key == KEY_RETURN) //'\n'
    {
        console.history.push(console.input_buffer);
        console.input_buffer.clear();
        return;
    }
    if (key == KEY_BACKSPACE)
    {
        if (control_pressed)
        {
            clear_input(console);
            return;
        }

        if (!console.input_buffer.empty())
        {
            console.input_buffer.pop_back();
	    }
        return;
    }
    if (key == KEY_LEFT)
    {
        if (console.caret_idx > 0) console.caret_idx -= 1;
        return;
    }
    if (key == KEY_RIGHT)
    {
        console.caret_idx += 1;
        if (console.caret_idx > console.input_buffer.size()) console.caret_idx = console.input_buffer.size();

        return;
    }

    // history stuff.
    if (key == KEY_UP)
    {
        if (!console.currently_scrolling_through_history_via_up_arrow)
        {
            if (console.history.size() == 0) return;
            
            console.currently_scrolling_through_history_via_up_arrow = true;
            console.index_of_history_entry_that_is_currently_displayed = console.history.index_of_latest_entry();

            console.input_buffer = console.history.back();
            return;
        }

        if (console.currently_scrolling_through_history_via_up_arrow)
        {
            // Move backwards in history
            if (console.index_of_history_entry_that_is_currently_displayed == 0)
            {
                // Wrap around to the most recent entry
                console.index_of_history_entry_that_is_currently_displayed = console.history.size() - 1;
            }
            else
            {
                console.index_of_history_entry_that_is_currently_displayed -= 1;
            }
                    // Retrieve history entry safely
            std::string retrieved_entry;
            if (console.history.get(console.index_of_history_entry_that_is_currently_displayed, retrieved_entry))
            {
                console.input_buffer = retrieved_entry;
            }
        }

        return;
    }

       
    if (key == KEY_DOWN)
    {
        if (console.currently_scrolling_through_history_via_up_arrow)
        {
            // Move forward through the ring buffer
            size_t next_index = console.history.wrap_index(console.index_of_history_entry_that_is_currently_displayed + 1);

            if (next_index == console.history.index_of_latest_entry() + 1)
            {
                // Reached the end of history, stop scrolling and clear input buffer
                console.currently_scrolling_through_history_via_up_arrow = false;
                console.input_buffer.clear();
            }
            else
            {
                console.index_of_history_entry_that_is_currently_displayed = next_index;

                // Retrieve the next history entry
                std::string retrieved_entry;
                if (console.history.get(console.index_of_history_entry_that_is_currently_displayed, retrieved_entry))
                {
                    console.input_buffer = retrieved_entry;
                }
            }
        }
    }



    char maybe_character = Keys::keycode_to_char(key, shift_pressed);
    if (maybe_character)
    {
        console.input_buffer.push_back(maybe_character);
    }
}

