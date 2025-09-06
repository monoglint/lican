#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>

#include "licanapi.hpp"
#include "core.hpp"

// Note: Index 0 is the command name
using t_command_data = std::vector<std::string>;

std::string get_line() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

// written by chatgpt because I'm lazy
t_command_data parse_string_command(const std::string& line) {
    t_command_data args;
    std::string buffer;

    auto push_buffer = [&](const std::string& buf) {
        if (buf.empty()) return;
        // Check for grouped short options (-rf)
        if (buf.size() > 1 && buf[0] == '-' && buf[1] != '-') {
            for (size_t i = 1; i < buf.size(); i++) {
                args.push_back(std::string("-") + buf[i]);
            }
        } else {
            args.push_back(buf);
        }
    };

    for (char c : line) {
        if (c == ' ') {
            push_buffer(buffer);
            buffer.clear();
            continue;
        }
        buffer += c;
    }

    push_buffer(buffer);

    return args;
}

// thanks copilot for autocompleting all of this for me...
t_command_data parse_c_style_command(int argc, char* argv[]) {
    t_command_data args;
    for (int i = 1; i < argc; i++) {
        std::string str = argv[i];

        if (str.size() > 1 && str[0] == '-' && str[1] != '-') {
            for (size_t j = 1; j < str.size(); j++) {
                args.push_back(std::string("-") + str[j]);
            }

            continue;
        }

        args.push_back(str);
    }

    return args;
}

bool HELP(const t_command_data& command) {
    std::cout << "commands:\n";
    std::cout << "help                                      Displays this help message.\n";
    std::cout << "build <path> <entry> <out> -<flags>       Builds the project at <path> with entry point <entry>.\n";
    std::cout << "write                                     Compiles the given code snippet. Flags are implicitly set for debug mode.\n";
    std::cout << "stress <chars>                            Compiles a given amount of characters and returns the compilation time.\n";
    std::cout << "flags                                     Lists all available build flags.\n";
    std::cout << "exit, quit                                Exits the program.\n";
    return true;
}

bool BUILD(const t_command_data& command) {
    if (command.size() < 4)
        return false;

    if (!std::filesystem::is_directory(command[1])) {
        std::cout << "The project path is not an existing directory.\n";
        return false;
    }
    if (!std::filesystem::exists(std::string(command[1]) + PREF_DIR_SEP + std::string(command[2]))) {
        std::cout << "The given entry point file name does not exist within the project directory.\n";
        return false;
    }
    if (!std::filesystem::is_directory(command[3])) {
        std::cout << "The output path is not an existing directory.\n";
        return false;
    }

    licanapi::liconfig_init config;
    config.project_path = command[1];
    config.entry_point_subpath = command[2];
    config.output_path = command[3];
    config.flag_list = command.size() > 4 ? std::vector<std::string>(command.begin() + 4, command.end()) : std::vector<std::string>();

    licanapi::build_project(config);

    return true;
}

bool WRITE(const t_command_data& command) {
    std::vector<std::string> flag_list = command.size() > 1 ? std::vector<std::string>(command.begin() + 1, command.end()) : std::vector<std::string>();

    std::cout << "write a code snippet:\n";
    std::string line = get_line();

    return licanapi::build_code(line, flag_list);
}

bool STRESS(const t_command_data& command) {
    if (command.size() != 2)
        return false;

    std::string buffer(static_cast<size_t>(std::stoull(command[1])), '/');

    return licanapi::build_code(buffer, {"-c"});
}

bool FLAGS(const t_command_data& command) {
    std::cout << "sorry guys, sorthands only:\n";
    std::cout << "dump-tokens     -t     Dumps the list of tokens generated during lexing.\n";
    std::cout << "dump-ast        -a     Dumps the AST generated during parsing.\n";
    std::cout << "dump-logs       -l     Dumps all logs generated during processing.\n";
    std::cout << "dump-chrono     -c     Dumps the amount of time it took each stage of the compiler to process.\n";

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
    if (cmd_name == "stress")
        return STRESS(command);
    if (cmd_name == "flags")
        return FLAGS(command);
    else
        return HELP(command);
}

int main(int argc, char* argv[]) {
    std::cout.setf(std::ios::unitbuf);

    if (argc > 1) {
        // Inverse bool so true means a successful exit code.
        const t_command_data command = parse_c_style_command(argc, argv);
        if (!process_command(command)) {
            std::cout << "Error processing command: " << command[0] << '\n';
            return 1;
        }

        std::cout << "finished\n";
        return 0;
    }

    std::cout << "Lican was invoked without arguments. Entering interactive mode.\n";
    std::cout << "Welcome to lican. Prompt 'help' for a list of commands.\n";
    while (true) {
        std::cout << "$l > ";

        const t_command_data command = parse_string_command(get_line());

        if (command.size() == 0)
            continue;

        if (command[0] == "exit" || command[0] == "quit")
            break;

        if (!process_command(command))
            std::cout << "Error processing command: " << command[0] << '\n';
    }

    std::cout << "finished\n";
    return 0;
}