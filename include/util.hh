/*

====================================================

Minor functions are are used in different locations of the compiler.

====================================================

*/

#pragma once

#include <string>

namespace liutil {
    inline bool is_whitespace(const char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    inline std::string indent_repeat(const uint8_t level) {
        std::string buffer;

        for (uint8_t i = 0; i < level; i++) {
            buffer += ".  ";
        }

        return buffer;
    }
}