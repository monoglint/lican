#include "core.hh"

bool core::frontend::init(liprocess& process) {
    bool add_success = process.add_file(process.config.entry_point_path);

    return add_success;
}

std::string core::lilog::pretty_debug(const liprocess& process) const {
    std::string log_label;

    switch (level) {
        case log_level::COMPILER_ERROR:
            log_label = "INTERNAL COMPILER ERROR"; break;
        case log_level::ERROR:
            log_label = "ERR"; break;
        case log_level::WARNING:
            log_label = "WAR"; break;
        default:
            log_label = "LOG"; break;
    }

    return std::string("[") + log_label + " - " + selection.pretty_debug(process) + " (" + process.file_list[selection.file_id].path + ")]: " + message + '\n' +
    "Selection: '" + process.sub_source_code(selection) + "'\n";
}

std::string core::lisel::pretty_debug(const liprocess& process) const {
    const core::liprocess::lifile& file = process.file_list[file_id];
    const core::t_pos line = file.get_line_of_position(start) + 1;
    const core::t_pos column = file.get_column_of_position(start) + 1;

    return std::string("[Line ") + std::to_string(line) + ", Col " + std::to_string(column) + ']';
}

core::t_pos core::liprocess::lifile::get_line_of_position(const t_pos position) const {
    auto it = std::upper_bound(line_marker_list.begin(), line_marker_list.end(), position);
    
    if (it == line_marker_list.begin()) return 0;
        return std::distance(line_marker_list.begin(), it);
}

core::t_pos core::liprocess::lifile::get_column_of_position(const t_pos position) const {
    auto it = std::upper_bound(line_marker_list.begin(), line_marker_list.end(), position);

    if (it == line_marker_list.begin())
        return position;
    else {
        auto prev_newline = *(it - 1);
        return position - prev_newline - 1;
    }
}

bool core::liprocess::add_file(const std::string& path) {
    if (file_list.size() > MAX_FILES) {
        add_log(lilog::log_level::COMPILER_ERROR, lisel(0, 0), "Too many files included.");
        return false;
    }
        
    std::ifstream file(path);

    if (!file.is_open()) {
        add_log(lilog::log_level::COMPILER_ERROR, lisel(0, 0), "Failed to open file..");
        return false;
    } 

    const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file_list.emplace_back(path, contents);

    return true;
}