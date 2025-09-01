// Contains all of the needed structures and data for the internal functionalities of the compiler.
// Used by all stages of the compiler.
// Holds the declarations for all of the stages of the compiler.
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <any>
#include <algorithm>
#include <iostream>

#include "licanapi.hpp"

namespace core {
    using t_file_id = int16_t;
    using t_pos = uint32_t;

    struct liprocess;

    // Displays information AS IS. Do not pivot to display to the user. All pivoting is handled implicitly.
    struct lisel {
        lisel(const t_pos start, const t_pos end)
            : start(start), end(end) {}

        lisel(const t_pos position)
            : start(position), end(position) {}

        lisel(const lisel& other0, const lisel& other1)
            : start(other0.start), end(other1.end) {}

        t_pos start;
        t_pos end;
        
        // Downshift operator
        inline lisel operator-(t_pos amount) const {
            return lisel(start - amount, end - amount);
        }
        // Upshift operator
        inline lisel operator+(t_pos amount) const {
            return lisel(start + amount, end + amount);
        }

        inline lisel& operator++() {
            start++;
            end++;

            return *this;
        }

        inline t_pos length() const {
            return end - start;
        }

        std::string pretty_debug(const liprocess& process) const;
    };

    struct lilog {
        enum class log_level : uint8_t {
            LOG,
            WARNING,
            ERROR
        };

        lilog(const log_level level, const lisel& selection, const std::string& message)
            : level(level), selection(selection), message(message) {}

        const log_level level;
        const lisel selection;
        const std::string message;

        std::string pretty_debug(const liprocess& process) const;
    };

    struct file_marker {
        file_marker(const t_file_id& source_file_id, const t_pos& position, const t_pos& char_index)
            : source_file_id(source_file_id), position(position), char_index(char_index) {}

        const t_file_id source_file_id;
        const t_pos position;
        const t_pos char_index;
    };

    struct lifile {
        lifile(const std::string& path)
            : path(path) {}

        const std::string path;
        std::vector<uint32_t> line_marker_list;
    };

    struct liprocess {
        liprocess(const licanapi::liconfig_init& config_init)
            : config(config_init) {}

        const licanapi::liconfig config;
        
        std::vector<lilog> logs;
        
        // Save file paths to be accessed by their ids.
        std::vector<lifile> file_list;

        std::string source_code;                        // Initialized by the preprocessor. Includes all imported files.
        std::vector<file_marker> file_marker_list;      // Marks checkpoints where files are imported into the main source code.

        // Data dump - avoids additional header dependencies. Decast as needed
        std::any dump_token_list;                   // std::vector<token>
        std::any dump_ast_root;                     // std::shared_ptr<ast::ast_root>
                                                    // so damn annoying but i need a wrapper that doesn't copy but is copyable and movable

        inline void add_log(const lilog::log_level level, const lisel& selection, const std::string& message) {
            logs.emplace_back(level, selection, message);
        }

        inline t_file_id add_file(const std::string& file_path) {
            file_list.push_back(file_path);
            return file_list.size() - 1;
        }
        
        inline file_marker get_marker_from_file_id(const t_file_id id) const {
            for (const auto& marker : file_marker_list) {
                if (marker.source_file_id == id)
                    return marker;
            }

            throw std::exception("Invalid file id.");
        }

        inline t_file_id get_file_id_from_char_pos(const t_pos pos) const {
            const file_marker& marker = get_marker_from_char_pos(pos);
            return marker.source_file_id;
        }

        inline file_marker get_marker_from_char_pos(const t_pos pos) const {
            for (size_t i = file_marker_list.size(); i > 0; i--) {
                if (file_marker_list[i - 1].position <= pos)
                    return file_marker_list[i - 1];
            }

            throw std::out_of_range("Position is invalid or there are no valid file markers in this process.");
        }

        // Gets the source line for code the character is in. The value must not be pivoted for accurate information.
        inline t_pos get_line_from_char_pos(const t_pos pos) const {
            auto marker = get_marker_from_char_pos(pos);
            auto& file = file_list[marker.source_file_id];

            for (size_t i = file.line_marker_list.size() - 1; i >= 0; i--) {
                if (file.line_marker_list[i] <= pos)
                    return i;
            }
            
            _STL_UNREACHABLE;
        }

        // Repositions the given selection in perspective to a marker.
        // If second argument is left empty, the marker will be the closest floored one to the given selection.

        inline lisel pivot_selection_to_marker(const lisel& selection) const {
            return pivot_selection_to_marker(selection, get_marker_from_char_pos(selection.start));
        }
        inline lisel pivot_selection_to_marker(const lisel& selection, const file_marker& marker) const {
            return selection - (marker.position - marker.char_index);
        }
        inline t_pos pivot_pos_to_marker(const t_pos pos) const {
            return pivot_pos_to_marker(pos, get_marker_from_char_pos(pos));
        }
        inline t_pos pivot_pos_to_marker(const t_pos pos, const file_marker& marker) const {
            return pos - (marker.position - marker.char_index);
        }

        inline std::string sub_source_code(const lisel& selection) const {
            return source_code.substr(selection.start, selection.end - selection.start + 1);
        }
    };

    // frontend
    namespace f {
        bool preprocess(liprocess& process);
        bool lex(liprocess& process);
        bool parse(liprocess& process);
    }

    // backend
    namespace b {

    }
}