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

    std::cout << "help\n";
    std::cout << "  Displays this help message.\n\n";

    std::cout << "build <entry_path> <out> -<flags>\n"; 
    std::cout << "  Builds the project at <path> with entry point <entry>.\n";
    std::cout << "  Assume all arguments are relative to cd.\n\n";

    std::cout << "write\n";
    std::cout << "  Compiles the given code snippet. Flags are implicitly set for debug mode.\n\n";

    std::cout << "stress <chars>\n";
    std::cout << "  Compiles a given amount of characters and returns the compilation time.\n\n";

    std::cout << "flags\n";
    std::cout << "  Lists all available build flags.\n\n";

    std::cout << "version\n";
    std::cout << "  Writes the current CLI and compiler version.\n\n";

    std::cout << "exit, quit\n";
    std::cout << "  Exits the program.\n\n";

    return true;
}

bool BUILD(const t_command_data& command) {
    if (command.size() < 3)
        return false;

    if (!std::filesystem::exists(std::string(command[1]))) {
        std::cout << "The given entry point file name does not exist within the project directory.\n";
        return false;
    }
    if (!std::filesystem::is_directory(command[2])) {
        std::cout << "The output path is not an existing directory.\n";
        return false;
    }

    licanapi::liconfig_init config;
    config.project_path = ""; // cd
    config.entry_point_subpath = command[1];
    config.output_path = command[2];
    config.flag_list = command.size() > 3 ? std::vector<std::string>(command.begin() + 3, command.end()) : std::vector<std::string>();

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

bool VERSION(const t_command_data& command) {
    std::cout << "lican v0.2.0-alpha\n";
    std::cout << "licancli v0.2.0-rc\n";
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
    if (cmd_name == "version")
        return VERSION(command);
    
    return HELP(command);
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // Inverse bool so true means a successful exit code.
        const t_command_data command = parse_c_style_command(argc, argv);
        bool process_success = process_command(command);
        
        std::cout << std::endl;
        
        if (!process_success) {
            std::cout << "Error processing command: " << command[0] << '\n';
            return 1;
        }

        return 0;
    }

    HELP({});

    /*
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
    */
    return 0;
}