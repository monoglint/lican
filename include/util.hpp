// Misc util that contains helpful functions for all stages of the compiler.
#pragma once

#include <string>

namespace liutil {
    inline bool is_whitespace(const char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }
}