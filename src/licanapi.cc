#include <fstream>
#include <iostream>
#include <filesystem>

#include "licanapi.hh"
#include "core.hh"
#include "token.hh"
#include "ast.hh"

const std::string TEMP_FOLDER_LOCATION = "LICANWRITE0";

std::pair<bool, std::chrono::milliseconds> measure_func(bool (*func)(core::liprocess&, const core::t_file_id), core::liprocess& process) {
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    bool result = func(process, 0);
    std::chrono::duration compilation_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);

    return std::make_pair(result, compilation_time);
}

bool run(core::liprocess& process) {
    if (!core::frontend::init(process))
        return false;

    // file_id 0 references the entry point file
    if (!core::frontend::lex(process, 0))
        return false;

    if (!core::frontend::parse(process, 0))
        return false;

    if (!core::frontend::semantic_analyze(process, 0))
        return false;

    return true;
}

bool run_chrono(core::liprocess& process) {
    if (!core::frontend::init(process)) return false;

    std::cout << "Starting lexical analysis:\n";
    auto lex = measure_func(core::frontend::lex, process);
    std::cout << "Lex time: " << lex.second.count() << "ms\n";
    if (!lex.first) 
        return false;

    std::cout << "Starting AST generation:\n";
    auto parse = measure_func(core::frontend::parse, process);
    std::cout << "Parse time: " << parse.second.count() << "ms\n"; 
    if (!parse.first) 
        return false;

    return true;
}

bool create_temp_file(const std::string& name, const std::string& content) {    
    std::ofstream temp(TEMP_FOLDER_LOCATION + '/' + name);

    if (!temp.is_open()) {
        std::cout << "Failed to generate temporary lican file.\n";
        return false;
    }

    temp.write(content.c_str(), content.length());
    temp.close();

    return true;
}

bool licanapi::build_project(const licanapi::liconfig_init& config) {
    std::cout << "Building (";

    if (config.flag_list.size() == 0)
        std::cout << "NO FLAGS";
    else
        for (auto& flag : config.flag_list) {
            std::cout << flag;
        }

    std::cout << ")\n";
    
    core::liprocess process(config);
    
    bool run_success = process.config._dump_chrono ? run_chrono(process) : run(process);

    if (process.config._dump_logs) {
        std::cout << "Logs:\n";
        for (auto& log : process.log_list) {
            std::cout << log.pretty_debug(process) << '\n';
        }
    }

    if (!run_success) {
        std::cout << "All debug info skipped. One or more processes resulted in termination of the compiler.\n";
        return false;
    }

    for (auto& file : process.file_list) {
        if (process.config._dump_token_list && file.dump_token_list.has_value()) {
            std::cout << "Tokens:\n";
            for (auto& token : std::any_cast<std::vector<core::token>>(file.dump_token_list)) {
                std::cout << token.pretty_debug(process) << '\n';
            }
        }

        if (process.config._dump_ast && file.dump_ast_arena.has_value()) {
            std::cout << "AST:\n";
            std::string buffer(0, ' ');
            auto ast_arena = std::any_cast<core::ast::ast_arena>(file.dump_ast_arena);
            ast_arena.pretty_debug(process, 0, buffer, 0);
            std::cout << buffer << '\n';
        }
    }

    return true;
}

bool licanapi::build_code(const std::string& code, const std::vector<std::string>& flag_list) {
    std::filesystem::create_directory(TEMP_FOLDER_LOCATION);

    create_temp_file("main.lican", code);

    licanapi::liconfig_init config;
    config.project_path = TEMP_FOLDER_LOCATION;
    config.entry_point_subpath = "main.lican";
    config.flag_list = flag_list;

    return build_project(config);
}