// Misc util that contains helpful functions for all stages of the compiler.
#pragma once

#include <string>

namespace liutil {
    inline bool is_whitespace(const char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    // Corrects all aspects of a file path from lican-standard to the style of the operating system.
    inline std::string reformat_file_path(std::string path) {
        std::replace(path.begin(), path.end(), '/', PREF_DIR_SEP);

        return path;
    }
}