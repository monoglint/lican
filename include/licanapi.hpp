// External api usage of the lican compiler.
#pragma once

#include "constants.hpp"

namespace licanapi {
    struct liconfig_init {
        liconfig_init() {}
        liconfig_init(const std::string& project_path, const std::string& entry_point_subpath)
            : project_path(project_path), entry_point_subpath(entry_point_subpath) {};
            
        std::string project_path;

        // Relative to project path
        std::string entry_point_subpath;
    };

    struct liconfig {
        liconfig(const liconfig_init init) : 
            project_path(init.project_path), 
            entry_point_path(init.project_path + PREF_DIR_SEP + init.entry_point_subpath) {}
            
        const std::string project_path;

        // Absolute path
        const std::string entry_point_path;
    };

    bool build_project(const liconfig_init& config);

    bool build_code(const std::string& code);
}