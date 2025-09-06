#include <unordered_set>

#include "core.hpp"
#include "ast.hpp"
#include "token.hpp"

// hello world

struct parse_state {
    parse_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), file(process.file_list[file_id]), token_list(std::any_cast<std::vector<core::token>&>(process.file_list[file_id].dump_token_list)) {}

    core::liprocess& process;

    core::t_file_id file_id;
    core::liprocess::lifile& file;

    std::vector<core::token>& token_list; // Ref to process property

    core::t_pos pos = 0;

    inline const core::token& now() const {
        return token_list.at(pos);
    }

    inline const core::token& next() {
        if (at_eof())
            return now();

        return token_list.at(pos++);
    }

    inline bool at_eof() const {
        return now().type == core::token_type::_EOF;
    }

    inline const core::token& expect(const core::token_type type, const std::string& error_message = "[No Info]") {
        const core::token& now = next();
        if (now.type != type)
            process.add_log(core::liprocess::lilog::log_level::ERROR, now.selection, "Unexpected token: " + error_message);
        
        return now;
    }

    inline core::lisel get_current_selection() const {
        if (at_eof())
            return std::prev(token_list.end())->selection;
        return now().selection;
    }
};

using t_p_expression_function = core::ast::p_expr(*)(parse_state& state);
using t_binary_operator_set = std::unordered_set<core::token_type>;

static const t_binary_operator_set set_scope_resolution = {
    core::token_type::DOUBLE_COLON
};

static const t_binary_operator_set set_member_access = {
    core::token_type::DOT
};

static const t_binary_operator_set set_exponential = {
    core::token_type::CARET
};

static const t_binary_operator_set set_multiplicative = {
    core::token_type::ASTERISK,
    core::token_type::SLASH,
    core::token_type::PERCENT
};

static const t_binary_operator_set set_additive = {
    core::token_type::PLUS,
    core::token_type::MINUS
};

static const t_binary_operator_set set_numeric_comparison = {
    core::token_type::LESS,
    core::token_type::LESS_EQUAL,
    core::token_type::GREATER,
    core::token_type::GREATER_EQUAL
};

static const t_binary_operator_set set_direct_comparison = {
    core::token_type::DOUBLE_EQUAL,
    core::token_type::BANG_EQUAL
};

static const t_binary_operator_set set_and = {
    core::token_type::DOUBLE_AMPERSAND
};

static const t_binary_operator_set set_or = {
    core::token_type::DOUBLE_PIPE
};

static const t_binary_operator_set set_assignment = {
    core::token_type::EQUAL,
    core::token_type::PLUS_EQUAL,
    core::token_type::MINUS_EQUAL,
    core::token_type::ASTERISK_EQUAL,
    core::token_type::SLASH_EQUAL,
    core::token_type::PERCENT_EQUAL,
    core::token_type::CARET_EQUAL
};

// Forward declarations
static core::ast::p_expr parse_expression(parse_state& state);
static core::ast::p_stmt parse_statement(parse_state& state);

static core::ast::p_expr binary_expression_left_associative(parse_state& state, const t_p_expression_function& lower, const t_binary_operator_set& set) {
    core::ast::p_expr left = lower(state);

    while (!state.at_eof() && set.find(state.now().type) != set.end()) {
        const core::token& opr = state.next();
        core::ast::p_expr right = lower(state);

        left = std::make_unique<core::ast::expr_binary>(
            core::lisel(left->selection, right->selection),
            std::move(left),
            std::move(right),
            opr
        );
    }

    return left;
}

static core::ast::p_expr binary_expression_right_associative(parse_state& state, const t_p_expression_function& lower, const t_binary_operator_set& set) {
    core::ast::p_expr left = lower(state);

    if (!state.at_eof() && set.find(state.now().type) != set.end()) {
        const core::token& opr = state.next();

        // recurse on *binary_expression* instead of just *lower*
        core::ast::p_expr right = binary_expression_right_associative(state, lower, set);
        
        return std::make_unique<core::ast::expr_binary>(
            core::lisel(left->selection, right->selection),
            std::move(left),
            std::move(right),
            opr
        );
    }

    return left;
}

static core::ast::p_expr parse_primary_expression(parse_state& state) {
    const core::token& tok = state.next();

    switch (tok.type) {
        case core::token_type::IDENTIFIER:
            return std::make_unique<core::ast::expr_identifier>(tok.selection);
        case core::token_type::NUMBER:
            return std::make_unique<core::ast::expr_literal>(tok.selection, core::ast::expr_literal::literal_type::NUMBER);
        case core::token_type::STRING:
            return std::make_unique<core::ast::expr_literal>(tok.selection, core::ast::expr_literal::literal_type::STRING);
        case core::token_type::CHAR:
            return std::make_unique<core::ast::expr_literal>(tok.selection, core::ast::expr_literal::literal_type::CHAR);
        case core::token_type::FALSE: [[fallthrough]];
        case core::token_type::TRUE:
            return std::make_unique<core::ast::expr_literal>(tok.selection, core::ast::expr_literal::literal_type::BOOL);
        case core::token_type::NIL:
            return std::make_unique<core::ast::expr_literal>(tok.selection, core::ast::expr_literal::literal_type::NIL);
        case core::token_type::LPAREN: {
            core::ast::p_expr expr = parse_expression(state);
            state.expect(core::token_type::RPAREN, "Expected ')' after expression.");
            return expr;
        }
        default:
            break;
    }

    state.process.add_log(core::liprocess::lilog::log_level::ERROR, tok.selection, "Unexpected token.");

    return std::make_unique<core::ast::expr_invalid>(tok.selection);
}

static core::ast::p_expr parse_scope_resolution(parse_state& state) {
    return binary_expression_left_associative(state, &parse_primary_expression, set_scope_resolution);
}

static core::ast::p_expr parse_member_access(parse_state& state) {
    return binary_expression_left_associative(state, &parse_scope_resolution, set_member_access);
}

static core::ast::p_expr parse_exponential(parse_state& state) {
    return binary_expression_right_associative(state, &parse_member_access, set_exponential);
}

static core::ast::p_expr parse_multiplicative(parse_state& state) {
    return binary_expression_left_associative(state, &parse_exponential, set_multiplicative);
}

static core::ast::p_expr parse_additive(parse_state& state) {
    return binary_expression_left_associative(state, &parse_multiplicative, set_additive);
}

static core::ast::p_expr parse_numeric_comparison(parse_state& state) {
    return binary_expression_left_associative(state, &parse_additive, set_numeric_comparison);
}

static core::ast::p_expr parse_direct_comparison(parse_state& state) {
    return binary_expression_left_associative(state, &parse_numeric_comparison, set_direct_comparison);
}

static core::ast::p_expr parse_and(parse_state& state) {
    return binary_expression_left_associative(state, &parse_direct_comparison, set_and);
}

static core::ast::p_expr parse_or(parse_state& state) {
    return binary_expression_left_associative(state, &parse_and, set_or);
}

static core::ast::p_expr parse_expr_ternary(parse_state& state) {
    core::ast::p_expr first = parse_or(state);
    if (state.now().type != core::token_type::QUESTION)
        return first;
    
    state.next();
    core::ast::p_expr second = parse_expression(state);
    state.expect(core::token_type::COLON, "Expected a colon.");
    core::ast::p_expr third = parse_expression(state);
    
    return std::make_unique<core::ast::expr_ternary>(core::lisel(first->selection, third->selection), first, second, third);
}

static core::ast::p_expr parse_assignment(parse_state& state) {
    return binary_expression_left_associative(state, &parse_expr_ternary, set_assignment);
}

static core::ast::p_expr parse_expression(parse_state& state) {
    return parse_assignment(state);
}

static std::unique_ptr<core::ast::stmt_if> parse_stmt_if(parse_state& state) {
    const core::token& if_token = state.next();
    core::ast::p_expr condition = parse_expression(state);
    core::ast::p_stmt consequent = parse_statement(state);
    core::ast::p_stmt alternate;

    if (state.now().type == core::token_type::ELSE) {
        state.next(); // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = std::make_unique<core::ast::stmt_none>(state.now().selection);

    return std::make_unique<core::ast::stmt_if>(core::lisel(if_token.selection, state.now().selection), condition, consequent, alternate);
}

static std::unique_ptr<core::ast::stmt_while> parse_stmt_while(parse_state& state) {
    const core::token& while_token = state.next();
    core::ast::p_expr condition = parse_expression(state);
    core::ast::p_stmt consequent = parse_statement(state);
    core::ast::p_stmt alternate;

    // In while loops, else's run if the condition fails on the first time.
    if (state.now().type == core::token_type::ELSE) {
        state.next(); // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = std::make_unique<core::ast::stmt_none>(state.now().selection);

    return std::make_unique<core::ast::stmt_while>(core::lisel(while_token.selection, state.now().selection), condition, consequent, alternate);
}

static std::unique_ptr<core::ast::stmt_body> parse_stmt_body(parse_state& state) {
    const core::token& brace_token = state.next();
    std::vector<core::ast::p_stmt> statement_list;

    while (!state.at_eof() && state.now().type != core::token_type::RBRACE) {
        statement_list.emplace_back(parse_statement(state));
    }

    if (state.at_eof())
        state.process.add_log(core::liprocess::lilog::log_level::ERROR, state.get_current_selection(), "Expected right brace, got EOF.");
    else
        state.next();
        
    return std::make_unique<core::ast::stmt_body>(core::lisel(brace_token.selection, state.get_current_selection()), statement_list);
}

static std::unique_ptr<core::ast::stmt_declaration> parse_stmt_variable_declaration(parse_state& state) {
    const core::token& if_token = state.next();
    std::vector<core::ast::qualifier> qualifiers = {};
    
    core::ast::p_expr name = parse_expr_ternary(state); // Do not let assignment binary expressions mess things up
    state.expect(core::token_type::EQUAL, "Expected an equals sign after declaration name.");
    core::ast::p_expr value = parse_expression(state);
    
    return std::make_unique<core::ast::stmt_declaration>(core::lisel(if_token.selection, state.get_current_selection()), name, value, qualifiers);
}

static core::ast::p_stmt parse_statement(parse_state& state) {
    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::IF: return parse_stmt_if(state);
        case core::token_type::WHILE: return parse_stmt_while(state);
        case core::token_type::LBRACE: return parse_stmt_body(state);
        case core::token_type::VAR: return parse_stmt_variable_declaration(state);
        default:     return std::make_unique<core::ast::stmt_wrapper>(parse_expression(state));
    }
}

bool core::frontend::parse(core::liprocess& process, const core::t_file_id file_id) {
    parse_state state(process, file_id);
    auto root = core::ast::ast_root();
    
    while (!state.at_eof()) {
        root.statement_list.emplace_back(parse_statement(state));
    }

    state.file.dump_ast_root = std::any(std::make_shared<core::ast::ast_root>(std::move(root)));

    return true;
}