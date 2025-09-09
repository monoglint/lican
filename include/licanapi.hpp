// External api usage of the lican compiler.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#define IS_LICANAPI_HPP

#ifdef IS_LICANAPI_HPP
static inline bool contains_flag(const std::vector<std::string>& flags, const std::string& flag) {
    return std::find(flags.begin(), flags.end(), flag) != flags.end();
}
#endif

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
        liconfig(const liconfig_init init) : 
            project_path(init.project_path), 
            entry_point_path(init.project_path + (init.project_path.length() > 0 ? "/" : "") + init.entry_point_subpath),
            output_path(init.output_path),
            _dump_token_list(contains_flag(init.flag_list, "-t")),
            _dump_ast(contains_flag(init.flag_list, "-a")),
            _dump_logs(contains_flag(init.flag_list, "-l")),
            _dump_chrono(contains_flag(init.flag_list, "-c")) {}
            
        const std::string project_path;
        const std::string output_path;

        // Absolute path
        const std::string entry_point_path;

        const bool _dump_token_list = false;
        const bool _dump_ast = false;
        const bool _dump_logs = false;
        const bool _dump_chrono = false;
    };

    bool build_project(const liconfig_init& config);

    bool build_code(const std::string& code, const std::vector<std::string>& flag_list = {});
}

#undef IS_LICANAPI_HPP