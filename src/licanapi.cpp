#include <fstream>
#include <iostream>
#include <filesystem>

#include "licanapi.hpp"
#include "core.hpp"
#include "token.hpp"
#include "ast.hpp"

const std::string TEMP_FOLDER_LOCATION = "TEMP_LICAN";

bool run(core::liprocess& process) {
    if (!core::f::preprocess(process))  return false;
    if (!core::f::lex(process))         return false;
    if (!core::f::parse(process))       return false;

    return true;
}

bool create_temp_file(const std::string& name, const std::string& content) {
    std::ofstream temp(TEMP_FOLDER_LOCATION + PREF_DIR_SEP + name);

    if (!temp.is_open()) {
        std::cout << "Failed to generate temporary lican file.\n";
        return false;
    }

    temp.write(content.c_str(), content.length());
    temp.close();

    return true;
}

bool licanapi::build_project(const licanapi::liconfig_init& config) {
    core::liprocess process(config);
    bool run_success = run(process);

    if (process.config._dump_logs) {
        std::cout << "Logs:\n";
        for (auto& log : process.logs) {
            std::cout << log.pretty_debug(process) << '\n';
        }
    }

    if (!run_success)
        return false;

    // Debug tokens if everything runs properly.
    if (process.config._dump_token_list && process.dump_token_list.has_value()) {
        std::cout << "Tokens:\n";
        for (auto& token : std::any_cast<std::vector<core::token>>(process.dump_token_list)) {
            std::cout << token.pretty_debug(process) << '\n';
        }
    }

    // Debug AST
    if (process.config._dump_ast && process.dump_ast_root.has_value()) {
        std::cout << "AST:\n";
        std::string buffer(0, ' ');
        auto ast_root_ptr = std::any_cast<std::shared_ptr<core::ast::ast_root>>(process.dump_ast_root);
        ast_root_ptr->pretty_debug(process, buffer, 0);
        std::cout << buffer << '\n';
    }

    return true;
}

bool licanapi::build_code(const std::string& code) {
    std::filesystem::create_directory(TEMP_FOLDER_LOCATION);

    create_temp_file("main.lican", code);

    // Test directories

    create_temp_file("math.lican", "m1\nm2\nworld #string world");
    create_temp_file("string.lican", "s1\ns2");

    licanapi::liconfig_init config;
    config.project_path = TEMP_FOLDER_LOCATION;
    config.entry_point_subpath = "main.lican";

    return build_project(config);
}