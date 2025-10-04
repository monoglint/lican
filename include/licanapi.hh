/*

====================================================

External API usage for the compiler.
The symbols in this file are useful to main.cpp and any project integrating this compiler.

CONTAINS liconfig_init and liconfig

IMPORTANT
The source file for this header contains the manager for the compilation pipeline.
All stages of compilation are called there.

====================================================

*/

#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace licanapi {
    // Interface version
    struct liconfig_init {
        liconfig_init() {}
            
        std::string project_path = "lican_temp_project";
        std::string output_path = std::string("lican_temp_project/out");

        // Relative to project path
        std::string entry_point_subpath = "main.lican";

        std::vector <std::string> flag_list = {};
    };

    // Scary internal version.
    struct liconfig {
        // Constructor implementation in source file so util functions are not unnecessarily passed on to includers.
        liconfig(const liconfig_init init);
            
        const std::string project_path;
        const std::string output_path;

        // Absolute path
        const std::string entry_point_path;

        const bool _dump_token_list = false;
        const bool _dump_ast = false;
        const bool _dump_logs = false;
        const bool _dump_chrono = false;
        const bool _show_cascading_logs = false;
    };

    bool build_project(const liconfig_init& config);

    bool build_code(const std::string& code, const std::vector<std::string>& flag_list = {});
}