#include "core.hpp"

bool core::frontend::init(liprocess& process) {
    bool add_success = process.add_file(process.config.entry_point_path);

    return add_success;
}

std::string core::liprocess::lilog::pretty_debug(const liprocess& process) const {
    const std::string log_label = level == log_level::ERROR ? "ERR" : level == log_level::WARNING ? "WAR" : "LOG";

    return std::string("[") + log_label + " - " + selection.pretty_debug(process) + " (" + process.file_list[selection.file_id].path + ")]: " + message;
}

std::string core::lisel::pretty_debug(const liprocess& process) const {
    return std::string("[Char ") + std::to_string(start) + ']';
}