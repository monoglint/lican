#include "core.hpp"

bool core::frontend::init(liprocess& process) {
    bool add_success = process.add_file(process.config.entry_point_path);
    std::cout << process.config.entry_point_path << '\n';

    return add_success;
}

std::string core::lilog::pretty_debug(const liprocess& process) const {
    const std::string log_label = level == log_level::ERROR ? "ERR" : level == log_level::WARNING ? "WAR" : "LOG";

    return std::string("[") + log_label + " - " + selection.pretty_debug(process) + " (" + process.file_list[selection.file_id].path + ")]: " + message + '\n' +
    "Selection: '" + process.sub_source_code(selection) + "'\n";
}

std::string core::lisel::pretty_debug(const liprocess& process) const {
    const core::liprocess::lifile& file = process.file_list[file_id];
    const core::t_pos line = file.get_line_of_position(start) + 1;
    const core::t_pos column = file.get_column_of_position(start) + 1;

    return std::string("[Line ") + std::to_string(line) + ", Col " + std::to_string(column) + ']';
}