#include <vector>
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <iostream>

#include "core.hpp"
#include "token.hpp"
#include "util.hpp"

#pragma region maps
static const std::unordered_map<std::string, core::token::token_type> keyword_map = {
    {"if", core::token::token_type::IF},
    {"else", core::token::token_type::ELSE},
    {"for", core::token::token_type::FOR},
    {"while", core::token::token_type::WHILE},
    {"return", core::token::token_type::RETURN},
    {"break", core::token::token_type::BREAK},
    {"continue", core::token::token_type::CONTINUE},
    {"class", core::token::token_type::CLASS},
    {"dec", core::token::token_type::DEC},
    {"var", core::token::token_type::VAR},
    {"true", core::token::token_type::TRUE},
    {"false", core::token::token_type::FALSE},
    {"null", core::token::token_type::NIL},
};
static const std::unordered_map<std::string, core::token::token_type> double_character_map = {
    {"&&", core::token::token_type::DOUBLE_AMPERSAND},
    {"||", core::token::token_type::DOUBLE_PIPE},
    {"::", core::token::token_type::DOUBLE_COLON},
    {"==", core::token::token_type::DOUBLE_EQUAL},
    {"!=", core::token::token_type::BANG_EQUAL},
    {"<=", core::token::token_type::LESS_EQUAL},
    {">=", core::token::token_type::GREATER_EQUAL},
    {"+=", core::token::token_type::PLUS_EQUAL},
    {"-=", core::token::token_type::MINUS_EQUAL},
    {"*=", core::token::token_type::ASTERISK_EQUAL},
    {"/=", core::token::token_type::SLASH_EQUAL},
    {"%=", core::token::token_type::PERCENT_EQUAL},
    {"^=", core::token::token_type::CARET_EQUAL},
};
static const std::unordered_map<char, core::token::token_type> character_map = {
    {'+', core::token::token_type::PLUS},
    {'-', core::token::token_type::MINUS},
    {'*', core::token::token_type::ASTERISK},
    {'/', core::token::token_type::SLASH},
    {'%', core::token::token_type::PERCENT},
    {'^', core::token::token_type::CARET},
    {'&', core::token::token_type::AMPERSAND},
    {'|', core::token::token_type::PIPE},
    {'?', core::token::token_type::QUESTION},
    {':', core::token::token_type::COLON},
    {'.', core::token::token_type::DOT},
    {'=', core::token::token_type::EQUAL},
    {'!', core::token::token_type::BANG},
    {'<', core::token::token_type::LESS},
    {'>', core::token::token_type::GREATER},
    {'(', core::token::token_type::LPAREN},
    {')', core::token::token_type::RPAREN},
    {'{', core::token::token_type::LBRACE},
    {'}', core::token::token_type::RBRACE},
    {',', core::token::token_type::COMMA},
};
#pragma endregion

struct lex_state {
    lex_state(core::liprocess& process)
        : process(process) {}

    core::liprocess& process;

    inline char now() const {
        return process.source_code[pos];
    }

    inline char next() {
        return process.source_code[pos++];
    }

    inline char peek(const core::t_pos amount = 1) const {
        return process.source_code[pos + amount];
    }

    inline bool at_eof() const {
        return pos >= process.source_code.length();
    }

    core::t_pos pos = 0;

    inline core::lisel get_selection() const {
        return core::lisel(pos);
    }
};

bool core::f::lex(core::liprocess& process) {
    lex_state state(process);

    auto token_list = std::vector<core::token>();

    while (!state.at_eof()) {
        char current_char = state.now();

        if (liutil::is_whitespace(current_char)) {
            state.next();
            continue;
        }

        // Strings
        if (current_char == '"') {
            auto start_pos = state.pos;

			state.next();
			
			while (!state.at_eof() && state.now() != '"') {
				state.next();
			}

			if (state.at_eof()) {
                process.add_log(core::lilog::log_level::ERROR, state.get_selection(), "Unterminated string literal.");

				break;
			}

            token_list.emplace_back(token::token_type::STRING, lisel(start_pos, state.pos));

            state.next();

            continue;
		}

        // Numbers
        if (std::isdigit(current_char) || (current_char == '.' && std::isdigit(state.peek(1)))) {
			auto start_pos = state.pos;
			auto used_dot = current_char == '.';

            state.next();

			while (!state.at_eof()) {
                if (std::isdigit(state.now())) {
                    state.next();
                    continue;
                }

                if (state.now() == '.') {
                    if (used_dot)
                        process.add_log(lilog::log_level::ERROR, lisel(state.pos), "A number can only have one decimal.");

                    used_dot = true;
                    state.next();
                    continue;
                }

                break;
			}

            if (state.peek(-1) == '.')
                process.add_log(lilog::log_level::ERROR, lisel(state.pos), "A number can't end with a deciaml point.");

            token_list.emplace_back(token::token_type::NUMBER, lisel(start_pos, state.pos - 1));

			continue;
		}

        // Identifiers
		if (std::isalnum(current_char) || current_char == '_') {
			auto start_pos = state.pos;

			while (!state.at_eof() && (std::isalnum(state.now()) || state.now() == '_')) {
				state.next();
			}

			auto selection = lisel(start_pos, state.pos - 1);

			auto identifier_string = process.sub_source_code(selection);
			
			if (keyword_map.find(identifier_string) != keyword_map.end()) {
                token_list.emplace_back(token::token_type::NUMBER, selection);
                continue;
			}

            token_list.emplace_back(token::token_type::IDENTIFIER, selection);

			continue;
		}

        // Double character tokens.
		if (!state.at_eof() && double_character_map.find(std::string{current_char, state.peek()}) != double_character_map.end()) {
            token_list.emplace_back(double_character_map.at(std::string{current_char, state.peek()}), lisel(state.pos, state.pos + 1));

            state.next(); state.next();
			continue;
		}

		// Single character tokens.
		if (character_map.find(current_char) != character_map.end()) {
            token_list.emplace_back(character_map.at(current_char), state.pos);

			state.next();
			continue;
		}

        process.add_log(core::lilog::log_level::ERROR, state.get_selection(), "Invalid token.");

        token_list.emplace_back(token::token_type::INVALID, state.get_selection());

        state.next();
    }

    process.dump_token_list = std::move(token_list);

    return true;
}