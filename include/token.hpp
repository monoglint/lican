#pragma once

#include "core.hpp"

namespace core {
    struct token {
        enum class token_type : uint16_t {
            INVALID,

            IDENTIFIER,
            NUMBER,
            STRING,
            CHAR,

            TRUE,
            FALSE,
            NIL,        // Lexer will just identify "null" as NIL

            IF,
            ELSE,
            FOR,
            WHILE,
            RETURN,

            BREAK,
            CONTINUE,
            CLASS,

            // Function declaration
            DEC,

            // Variable declaration
            VAR,

            COLON,

            PLUS,
            MINUS,
            ASTERISK,
            SLASH,
            PERCENT,
            CARET,

            COMMA,
            DOT,

            AMPERSAND,
            PIPE,
            QUESTION,

            DOUBLE_AMPERSAND,
            DOUBLE_PIPE,
            DOUBLE_COLON,

            EQUAL,
            DOUBLE_EQUAL,
            BANG,
            BANG_EQUAL,
            LESS,
            LESS_EQUAL,
            GREATER,
            GREATER_EQUAL,

            PLUS_EQUAL,
            MINUS_EQUAL,
            ASTERISK_EQUAL,
            SLASH_EQUAL,
            PERCENT_EQUAL,
            CARET_EQUAL,

            LPAREN,
            RPAREN,
            LBRACE,
            RBRACE,
        };

        token(token_type type, core::lisel selection)
            : type(type), selection(selection) {}

        token_type type;
        core::lisel selection;

        inline std::string pretty_debug(const liprocess& process) {
            const file_marker& marker = process.get_marker_from_char_pos(selection.start);

            return std::string("[") + selection.pretty_debug(process) + " (" + process.file_list[marker.source_file_id].path + ")]:\t" + (type == token_type::INVALID ? "INVALID" : process.sub_source_code(selection));
        }
    };
}