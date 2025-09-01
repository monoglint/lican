#include <iostream>
#include <string>
#include <filesystem>

#include "licanapi.hpp"
#include "core.hpp"

// Note: Index 0 is the command name
using t_command_data = std::vector<std::string>;

std::string get_line() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// Parse through a line and separate by spaces.
t_command_data separate_command(const std::string& line) {
    t_command_data args;
    std::string buffer;
    for (char c : line) {
        if (c == ' ') {
            if (buffer.length() > 0) {
                args.push_back(buffer);
                buffer.clear();
            }
        }
        else {
            buffer += c;
        }
    }

    if (buffer.length() > 0)
        args.push_back(buffer);
    
    return args;
}

t_command_data c_args_to_command_data(int argc, char* argv[]) {
    t_command_data args;
    for (int i = 1; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }
    return args;
}

bool HELP(const t_command_data& command) {
    std::cout << "Commands:\n";
    std::cout << "build <path> <entry>              Builds the project at <path> with entry point <entry>.\n";
    std::cout << "write                             Compiles the given code snippet.\n";
    std::cout << "help                              Displays this help message.\n";
    std::cout << "exit, quit                        Exits the program.\n";
    return true;
}

bool WRITE(const t_command_data& command) {
    std::cout << "Write a code snippet:\n";
    std::string line = get_line();

    return licanapi::build_code(line);
}

bool BUILD(const t_command_data& command) {
    if (command.size() != 3)
        return false;

    if (!std::filesystem::is_directory(command[1])) {
        std::cerr << "Project path is not a directory.\n";
        return false;
    }
    if (!std::filesystem::exists(std::string(command[1]) + PREF_DIR_SEP + std::string(command[2]))) {
        std::cerr << "Entry point file does not exist.\n";
        return false;
    }

    licanapi::liconfig_init config;
    config.project_path = command[1];
    config.entry_point_subpath = command[2];

    licanapi::build_project(config);

    return true;
}

bool process_command(const t_command_data& command) {
    std::string cmd_name = command[0];

    if (cmd_name == "help")
        return HELP(command);
    if (cmd_name == "build")
        return BUILD(command);
    if (cmd_name == "write")
        return WRITE(command);
    else
        return HELP(command);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // Inverse bool so true means a successful exit code.
        const t_command_data command = c_args_to_command_data(argc, argv);
        return !process_command(command);
    }

    std::cout << "lican.exe ran without arguments. Entering interactive mode.\n";
    std::cout << "Welcome to lican. Prompt 'help' for a list of commands.\n";
    while (true) {
        std::cout << "$l > ";

        const t_command_data command = separate_command(get_line());

        if (command.size() == 0)
            continue;

        if (command[0] == "exit" || command[0] == "quit")
            break;

        if (!process_command(command))
            std::cout << "Error processing command: " << command[0] << '\n';
    }

    return 0;
}