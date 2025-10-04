#pragma once

#include "core.hh"

namespace core {
    enum class token_type : uint16_t {
        INVALID,
        _EOF,

        IDENTIFIER,
        NUMBER,
        STRING,
        CHAR,

        MUT,

        STRUCT,
        COMPONENT,

        TRUE,
        FALSE,
        NIL,
        
        IF,
        ELSE,
        FOR,
        WHILE,
        RETURN,

        BREAK,
        CONTINUE,

        // Variable and function declaration
        DEC,
        TYPEDEC,
        
        USE,

        COLON,
        POUND,
        
        MODULE,
        
        PLUS,
        MINUS,
        ASTERISK,
        SLASH,
        PERCENT,
        CARET,

        COMMA,
        DOT,

        AT,

        AMPERSAND,
        PIPE,
        QUESTION,

        DOUBLE_AMPERSAND,
        DOUBLE_PIPE,
        DOUBLE_COLON,
        DOUBLE_PLUS,
        DOUBLE_MINUS,

        EQUAL,
        DOUBLE_EQUAL,
        BANG,
        BANG_EQUAL,
        LESS_EQUAL,
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
        LARROW,
        RARROW,
        LSQUARE,
        RSQUARE,
    };

    struct token {
        token(const token_type& type, const core::lisel& selection)
            : type(type), selection(selection) {}

        const token_type type;
        const core::lisel selection;

        inline std::string pretty_debug(const liprocess& process) {
            return std::string("[") + selection.pretty_debug(process) + " (" + process.file_list[selection.file_id].path + ")]:\t" + (type == token_type::INVALID ? "INVALID" : (type == token_type::_EOF ? "EOF" : process.sub_source_code(selection)));
        }
    };
}