#include <vector>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <iostream>

#include "core.hpp"
#include "token.hpp"
#include "util.hpp"

static const std::unordered_map<std::string, core::token_type> keyword_map = {
    {"if", core::token_type::IF},
    {"else", core::token_type::ELSE},
    {"for", core::token_type::FOR},
    {"while", core::token_type::WHILE},
    {"return", core::token_type::RETURN},
    {"break", core::token_type::BREAK},
    {"continue", core::token_type::CONTINUE},
    {"dec", core::token_type::DEC},
    {"typedec", core::token_type::TYPEDEC},
    {"true", core::token_type::TRUE},
    {"false", core::token_type::FALSE},
    {"nil", core::token_type::NIL},
    {"use", core::token_type::USE},
    {"struct", core::token_type::STRUCT},
    {"component", core::token_type::COMPONENT},
    {"module", core::token_type::MODULE},
};

static const std::unordered_map<std::string, core::token_type> double_character_map = {
    {"&&", core::token_type::DOUBLE_AMPERSAND},
    {"||", core::token_type::DOUBLE_PIPE},
    {"::", core::token_type::DOUBLE_COLON},
    {"==", core::token_type::DOUBLE_EQUAL},
    {"!=", core::token_type::BANG_EQUAL},
    {"<=", core::token_type::LESS_EQUAL},
    {">=", core::token_type::GREATER_EQUAL},
    {"+=", core::token_type::PLUS_EQUAL},
    {"-=", core::token_type::MINUS_EQUAL},
    {"*=", core::token_type::ASTERISK_EQUAL},
    {"/=", core::token_type::SLASH_EQUAL},
    {"%=", core::token_type::PERCENT_EQUAL},
    {"^=", core::token_type::CARET_EQUAL},
    {"++", core::token_type::DOUBLE_PLUS},
    {"--", core::token_type::DOUBLE_MINUS},
};

static const std::unordered_map<char, core::token_type> character_map = {
    {'+', core::token_type::PLUS},
    {'-', core::token_type::MINUS},
    {'*', core::token_type::ASTERISK},
    {'/', core::token_type::SLASH},
    {'%', core::token_type::PERCENT},
    {'^', core::token_type::CARET},
    {'&', core::token_type::AMPERSAND},
    {'|', core::token_type::PIPE},
    {'?', core::token_type::QUESTION},
    {':', core::token_type::COLON},
    {'.', core::token_type::DOT},
    {'=', core::token_type::EQUAL},
    {'!', core::token_type::BANG},
    {'(', core::token_type::LPAREN},
    {')', core::token_type::RPAREN},
    {'{', core::token_type::LBRACE},
    {'}', core::token_type::RBRACE},
    {',', core::token_type::COMMA},
    {'<', core::token_type::LARROW},
    {'>', core::token_type::RARROW},
    {'@', core::token_type::AT},
    {'#', core::token_type::POUND},
};

struct lex_state {
    lex_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), file(process.file_list[file_id]) {}

    core::liprocess& process;
    
    const core::t_file_id file_id;
    core::liprocess::lifile& file;

    inline char now() const {
        return file.source_code[pos];
    }

    inline char next() {
        if (now() == '\n') 
            file.line_marker_list.push_back(pos);

        return file.source_code[pos++];
    }

    inline char peek(const core::t_pos amount = 1) const {
        if (!is_peek_safe(amount))
            return file.source_code[file.source_code.length() - 1];
        
        return file.source_code[pos + amount];
    }
    
    inline bool is_peek_safe(const core::t_pos amount = 1) const {
        return pos + amount < file.source_code.length();
    }

    inline bool at_eof() const {
        return pos >= file.source_code.length();
    }

    core::t_pos pos = 0;

    inline core::lisel get_selection() const {
        return core::lisel(file_id, pos);
    }
};

bool core::frontend::lex(core::liprocess& process, const core::t_file_id file_id) {
    lex_state state(process, file_id);

    std::vector<core::token> token_list;
    token_list.reserve(state.file.source_code.length() / 1.5);

    while (!state.at_eof()) {
        char current_char = state.now();

        if (liutil::is_whitespace(current_char)) {
            state.next();
            continue;
        }

        if (current_char == ';') {
            state.next();
            if (state.now() == '*') {
                state.next();
                while (!state.at_eof() && !(state.now() == '*' && state.peek(1) == ';'))
                    state.next();

                if (state.at_eof())
                    process.add_log(lilog::log_level::ERROR, state.get_selection(), "Unending multiline comment.");
                else {
                    state.next(); // skip '*'
                    state.next(); // skip '#'
                }
            }
            else {
                // Single-line comment: skip until end of line or file
                while (!state.at_eof() && state.now() != '\n')
                    state.next();
                if (!state.at_eof())
                    state.next(); // skip the newline character
            }
            continue;
        }

        // Strings
        if (current_char == '"') {
            core::t_pos start_pos = state.pos;

			state.next();
			
			while (!state.at_eof() && state.now() != '"') {
				state.next();
			}

			if (state.at_eof()) {
                process.add_log(lilog::log_level::ERROR, state.get_selection(), "Unterminated string literal.");

				break;
			}

            token_list.emplace_back(token_type::STRING, lisel(state.file_id, start_pos, state.pos));

            state.next();

            continue;
		}

        // Numbers
        if (std::isdigit(current_char) || (current_char == '.' && std::isdigit(state.peek(1)))) {
			core::t_pos start_pos = state.pos;
			bool used_dot = current_char == '.';

            state.next();

			while (!state.at_eof()) {
                if (std::isdigit(state.now())) {
                    state.next();
                    continue;
                }

                if (state.now() == '.') {
                    if (used_dot)
                        process.add_log(lilog::log_level::ERROR, lisel(state.file_id, state.pos), "A number can only have one decimal.");

                    used_dot = true;
                    state.next();
                    continue;
                }

                break;
			}

            if (state.peek(-1) == '.')
                process.add_log(lilog::log_level::ERROR, lisel(state.file_id, state.pos), "A number can't end with a deciaml point.");

            token_list.emplace_back(token_type::NUMBER, lisel(state.file_id, start_pos, state.pos - 1));

			continue;
		}

        // Identifiers
		if (std::isalnum(current_char) || current_char == '_') {
			core::t_pos start_pos = state.pos;

			while (!state.at_eof() && (std::isalnum(state.now()) || state.now() == '_')) {
				state.next();
			}

			lisel selection(state.file_id, start_pos, state.pos - 1);

			std::string identifier_string = process.sub_source_code(selection);
			
			if (keyword_map.find(identifier_string) != keyword_map.end()) {
                token_list.emplace_back(keyword_map.at(identifier_string), selection);
                continue;
			}

            token_list.emplace_back(token_type::IDENTIFIER, selection);

			continue;
		}

        // Double character tokens.
		if (!state.at_eof() && double_character_map.find(std::string{current_char, state.peek()}) != double_character_map.end()) {
            token_list.emplace_back(double_character_map.at(std::string{current_char, state.peek()}), lisel(state.file_id, state.pos, state.pos + 1));

            state.next(); state.next();
			continue;
		}

		// Single character tokens.
		if (character_map.find(current_char) != character_map.end()) {
            token_list.emplace_back(character_map.at(current_char), state.get_selection());

			state.next();
			continue;
		}

        process.add_log(core::lilog::log_level::ERROR, state.get_selection(), "Invalid token.");

        token_list.emplace_back(token_type::INVALID, state.get_selection());

        state.next();
    }

    token_list.emplace_back(token_type::_EOF, state.get_selection());
    state.file.dump_token_list = std::move(token_list);

    return true;
}