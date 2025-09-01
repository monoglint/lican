#include "core.hpp"

std::string core::lilog::pretty_debug(const liprocess& process) const {
    const std::string log_label = level == log_level::ERROR ? "ERR" : level == log_level::WARNING ? "WAR" : "LOG";

    const file_marker& marker = process.get_marker_from_char_pos(selection.start);

    return std::string("[") + log_label + " - " + selection.pretty_debug(process) + " (" + process.file_list[marker.source_file_id].path + ")]: " + message;
}

std::string core::lisel::pretty_debug(const liprocess& process) const {
    t_pos line = process.get_line_from_char_pos(start) + 1;

    return std::string("[Line ") + std::to_string(line) + ']';
}