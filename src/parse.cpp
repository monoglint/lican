#include <unordered_set>

#include "core.hpp"
#include "ast.hpp"
#include "token.hpp"

struct parse_state {
    parse_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), file(process.file_list[file_id]), tokens(std::any_cast<std::vector<core::token>&>(process.file_list[file_id].dump_token_list)) {}

    core::liprocess& process;

    core::t_file_id file_id;
    core::liprocess::lifile& file;

    std::vector<core::token>& tokens; // Ref to process property

    core::t_pos pos = 0;

    inline const core::token& now() const {
        return tokens.at(pos);
    }

    inline core::token& next() {
        return tokens.at(pos++);
    }

    inline bool at_eof() const {
        return pos >= tokens.size();
    }

    inline const core::token& expect(const core::token::token_type type, const std::string& error_message = "[No Info]") {
        auto& now = next();
        if (now.type != type)
            process.add_log(core::liprocess::lilog::log_level::ERROR, now.selection, "Unexpected token: " + error_message);
        
        return now;
    }
};

using t_p_expression_function = core::ast::p_expr(*)(parse_state& state);
using t_binary_operator_set = std::unordered_set<core::token::token_type>;

#pragma region binary operator sets
static const t_binary_operator_set set_scope_resolution = {
    core::token::token_type::DOUBLE_COLON
};

static const t_binary_operator_set set_member_access = {
    core::token::token_type::DOT
};

static const t_binary_operator_set set_exponential = {
    core::token::token_type::CARET
};

static const t_binary_operator_set set_multiplicative = {
    core::token::token_type::ASTERISK,
    core::token::token_type::SLASH,
    core::token::token_type::PERCENT
};

static const t_binary_operator_set set_additive = {
    core::token::token_type::PLUS,
    core::token::token_type::MINUS
};

static const t_binary_operator_set set_numeric_comparison = {
    core::token::token_type::LESS,
    core::token::token_type::LESS_EQUAL,
    core::token::token_type::GREATER,
    core::token::token_type::GREATER_EQUAL
};

static const t_binary_operator_set set_direct_comparison = {
    core::token::token_type::DOUBLE_EQUAL,
    core::token::token_type::BANG_EQUAL
};

static const t_binary_operator_set set_and = {
    core::token::token_type::DOUBLE_AMPERSAND
};

static const t_binary_operator_set set_or = {
    core::token::token_type::DOUBLE_PIPE
};

static const t_binary_operator_set set_assignment = {
    core::token::token_type::EQUAL,
    core::token::token_type::PLUS_EQUAL,
    core::token::token_type::MINUS_EQUAL,
    core::token::token_type::ASTERISK_EQUAL,
    core::token::token_type::SLASH_EQUAL,
    core::token::token_type::PERCENT_EQUAL,
    core::token::token_type::CARET_EQUAL
};

#pragma endregion
#pragma region binary functions

// Forward declaration
static core::ast::p_expr parse_expression(parse_state& state);

static core::ast::p_expr binary_expression(parse_state& state, const t_p_expression_function& lower, const t_binary_operator_set& set) {
    auto left = lower(state);

    while (!state.at_eof() && set.find(state.now().type) != set.end()) {
        auto& opr = state.next();
        auto right = lower(state);

        left = std::make_unique<core::ast::expr_binary>(
            core::lisel(left->selection, right->selection),
            std::move(left),
            std::move(right),
            opr
        );
    }

    return left;
}

static core::ast::p_expr binary_expression_right(parse_state& state, const t_p_expression_function& lower, const t_binary_operator_set& set) {
    auto left = lower(state);

    if (!state.at_eof() && set.find(state.now().type) != set.end()) {
        auto& opr = state.next();

        // recurse on *binary_expression* instead of just *lower*
        auto right = binary_expression_right(state, lower, set);

        return std::make_unique<core::ast::expr_binary>(
            core::lisel(left->selection, right->selection),
            std::move(left),
            std::move(right),
            opr
        );
    }

    return left;
}
#pragma endregion

static core::ast::p_expr parse_primary_expression(parse_state& state) {
    auto& now = state.next();

    switch (now.type) {
        case core::token::token_type::IDENTIFIER:
            return std::make_unique<core::ast::expr_identifier>(now.selection);
        case core::token::token_type::NUMBER:
            return std::make_unique<core::ast::expr_literal>(now.selection, core::ast::expr_literal::literal_type::NUMBER);
        case core::token::token_type::STRING:
            return std::make_unique<core::ast::expr_literal>(now.selection, core::ast::expr_literal::literal_type::STRING);
        case core::token::token_type::CHAR:
            return std::make_unique<core::ast::expr_literal>(now.selection, core::ast::expr_literal::literal_type::CHAR);
        case core::token::token_type::LPAREN: {
            auto expr = parse_expression(state);
            state.expect(core::token::token_type::RPAREN, "Expected ')' after expression.");
            return expr;
        }
        default:
            break;
    }

    state.process.add_log(core::liprocess::lilog::log_level::ERROR, now.selection, "Unexpected token.");

    return std::make_unique<core::ast::expr_invalid>(now.selection);
}

#pragma region binary shit
static core::ast::p_expr parse_scope_resolution(parse_state& state) {
    return binary_expression(state, &parse_primary_expression, set_scope_resolution);
}

static core::ast::p_expr parse_member_access(parse_state& state) {
    return binary_expression(state, &parse_scope_resolution, set_member_access);
}

static core::ast::p_expr parse_exponential(parse_state& state) {
    return binary_expression_right(state, &parse_member_access, set_exponential);
}

static core::ast::p_expr parse_multiplicative(parse_state& state) {
    return binary_expression(state, &parse_exponential, set_multiplicative);
}

static core::ast::p_expr parse_additive(parse_state& state) {
    return binary_expression(state, &parse_multiplicative, set_additive);
}

static core::ast::p_expr parse_numeric_comparison(parse_state& state) {
    return binary_expression(state, &parse_additive, set_numeric_comparison);
}

static core::ast::p_expr parse_direct_comparison(parse_state& state) {
    return binary_expression(state, &parse_numeric_comparison, set_direct_comparison);
}

static core::ast::p_expr parse_and(parse_state& state) {
    return binary_expression(state, &parse_direct_comparison, set_and);
}

static core::ast::p_expr parse_or(parse_state& state) {
    return binary_expression(state, &parse_and, set_or);
}

static core::ast::p_expr parse_assignment(parse_state& state) {
    return binary_expression(state, &parse_or, set_assignment);
}
#pragma endregion

static core::ast::p_expr parse_expression(parse_state& state) {
    return parse_assignment(state);
}

static core::ast::p_stmt parse_statement(parse_state& state) {
    return std::make_unique<core::ast::stmt_wrapper>(parse_expression(state));
}

bool core::f::parse(core::liprocess& process, const core::t_file_id file_id) {
    parse_state state(process, file_id);
    auto root = core::ast::ast_root();
    
    while (!state.at_eof()) {
        auto stmt = parse_statement(state);
        root.statements.push_back(std::move(stmt));
    }

    state.file.dump_ast_root = std::any(std::make_shared<core::ast::ast_root>(std::move(root)));

    return true;
}