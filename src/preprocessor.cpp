#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <iostream>

#include "core.hpp"
#include "token.hpp"
#include "util.hpp"

struct file_stack_frame {
    file_stack_frame(const core::t_file_id file_id, const std::string& path, const core::t_pos file_contents_length)
        : file_id(file_id), path(path), chars_left(file_contents_length), per_file_pos(0) {}

    const core::t_file_id file_id;
    const std::string path;
    core::t_pos chars_left;
    core::t_pos per_file_pos;           // Keep track of the position if the pointer in the "currently processing" file. 
};

struct preprocess_state {
    preprocess_state(core::liprocess& process)
        : process(process) {}

    core::liprocess& process;

    // Not safe to modify independently. Use next().
    core::t_pos pos = 0;

    inline char now() const {
        return process.source_code[pos];
    }

    inline bool at_eof() const {
        return pos >= process.source_code.length();
    }

    inline char next() {
        file_stack_frame& top_file = peek_file_stack();
        top_file.per_file_pos++;
        top_file.chars_left--;

        if (now() == '\n')
            process.file_list[top_file.file_id].line_marker_list.emplace_back(pos);

        if (top_file.chars_left == 0 && file_stack.size() > 1) {
            file_stack_frame& nested_file = peek_file_stack(1);

            process.file_marker_list.emplace_back(nested_file.file_id, pos + 1, nested_file.per_file_pos);

            file_stack.pop_back();
        }

        return process.source_code[pos++];
    }

    inline file_stack_frame& peek_file_stack(size_t depth = 0) {
        return file_stack.at(file_stack.size() - 1 - depth);
    }

    // Returns whether or not the operation was a success.
    inline bool push_file(const std::string& path, const core::lisel& error_selection = (0)) {
        for (const file_stack_frame& frame : file_stack) {
            if (frame.path == path) {
                process.add_log(core::lilog::log_level::WARNING, error_selection, "[push_file]: Potential inclusion recursion.");
                return true;
            }
        }
        
        core::t_file_id file_id = process.add_file(path);
        process.file_list[file_id].line_marker_list.emplace_back(pos);

        std::ifstream file_stream(path);

        if (!file_stream.is_open()) {
            process.add_log(core::lilog::log_level::ERROR, error_selection, "[push_file]: Failed to open '" + path + "'.");
            return false;
        }
            

        // No way in hell am I remembering this
        std::string file_contents{std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>()};

        file_stack.emplace_back(file_id, path, file_contents.length());
        process.file_marker_list.emplace_back(file_id, pos, 0);

        process.source_code.insert(pos, file_contents);

        return true;
    }

    std::vector<file_stack_frame> file_stack;
};

bool core::f::preprocess(core::liprocess& process) {
    preprocess_state state(process);

    // Push our primary file into the stack first since that is what we're processing.
    // This will initialize the source_code string and get things started.
    state.push_file(process.config.entry_point_path);

    while (!state.at_eof()) {
        char current_char = state.now();

        if (liutil::is_whitespace(current_char)) {
            state.next();
            continue;
        }

        // Detect preprocessor tags
        if (current_char == '#') {
            t_pos start_pos = state.pos;

            state.next();

            while (!state.at_eof() && (std::isalnum(state.now()) || state.now() == '/')) {
                state.next();
            }

            const lisel selection(start_pos + 1, state.pos - 1);  // Remember, start pos does not include the #.
            std::string import_path = process.sub_source_code(selection);

            import_path = liutil::reformat_file_path(import_path);
            import_path.insert(0, process.config.project_path + PREF_DIR_SEP);
            import_path.append(".lican");

            // Amount to backtrack the pointer after erasing the macro.
            t_pos subtraction_amount = (state.pos) - start_pos;

            process.source_code.erase(start_pos, subtraction_amount);
            state.pos -= subtraction_amount;

            bool push_success = state.push_file(import_path); 

            if (!push_success) {
                process.add_log(lilog::log_level::ERROR, lisel(start_pos, state.pos - 1), "File inclusion failed. Terminating.");
                return false;
            }

            continue;
        }

        // Escape strings
        if (state.now() == '"') {
			state.next();
			
			while (!state.at_eof() && state.now() != '"') {
				state.next();
			}

			if (!state.at_eof()) {
				state.next();
			}

            continue;
		}

        state.next();
    }

    return true;
}