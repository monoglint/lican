#include <unordered_set>

#include "core.hpp"
#include "ast.hpp"
#include "token.hpp"

struct parse_state;

using t_p_expression_function = core::ast::p_expr(*)(parse_state& state);
using t_operator_set = std::unordered_set<core::token_type>;

static const t_operator_set set_scope_resolution = {
    core::token_type::DOUBLE_COLON
};

static const t_operator_set set_member_access = {
    core::token_type::DOT
};

static const t_operator_set set_unary_post = {
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_operator_set set_unary_pre = {
    core::token_type::MINUS,
    core::token_type::BANG,
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_operator_set set_exponential = {
    core::token_type::CARET
};

static const t_operator_set set_multiplicative = {
    core::token_type::ASTERISK,
    core::token_type::SLASH,
    core::token_type::PERCENT
};

static const t_operator_set set_additive = {
    core::token_type::PLUS,
    core::token_type::MINUS
};

static const t_operator_set set_numeric_comparison = {
    core::token_type::LARROW,
    core::token_type::LESS_EQUAL,
    core::token_type::RARROW,
    core::token_type::GREATER_EQUAL
};

static const t_operator_set set_direct_comparison = {
    core::token_type::DOUBLE_EQUAL,
    core::token_type::BANG_EQUAL
};

static const t_operator_set set_and = {
    core::token_type::DOUBLE_AMPERSAND
};

static const t_operator_set set_or = {
    core::token_type::DOUBLE_PIPE
};

static const t_operator_set set_assignment = {
    core::token_type::EQUAL,
    core::token_type::PLUS_EQUAL,
    core::token_type::MINUS_EQUAL,
    core::token_type::ASTERISK_EQUAL,
    core::token_type::SLASH_EQUAL,
    core::token_type::PERCENT_EQUAL,
    core::token_type::CARET_EQUAL
};

struct parse_state {
    parse_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), file(process.file_list[file_id]), token_list(std::any_cast<const std::vector<core::token>&>(process.file_list[file_id].dump_token_list)) {}

    core::liprocess& process;

    const core::t_file_id file_id;
    core::liprocess::lifile& file;

    const std::vector<core::token>& token_list; // Ref to process property

    core::t_pos pos = 0;

    inline const core::token& now() const {
        return token_list.at(pos);
    }

    inline const core::token& next() {
        if (at_eof())
            return now();

        return token_list.at(pos++);
    }
    
    inline const core::token& peek(const core::t_pos amount = 1) const {
        if (!is_peek_safe(amount))
            return token_list[token_list.size() - 1];
        
        return token_list[pos + amount];
    }
    
    inline bool is_peek_safe(const core::t_pos amount = 1) const {
        return pos + amount < token_list.size() - 1;
    }

    inline bool at_eof() const {
        return pos >= token_list.size() - 1;
    }

    inline const core::token& expect(const core::token_type type, const std::string& error_message = "[No Info]") {
        const core::token& now = next();
        if (now.type != type)
            process.add_log(core::lilog::log_level::ERROR, now.selection, "Unexpected token: " + error_message);
        
        return now;
    }
};

// Forward declarations
static core::ast::p_expr parse_expression(parse_state& state);
static core::ast::p_expr parse_scope_resolution(parse_state& state);
static core::ast::p_stmt parse_statement(parse_state& state);
static std::unique_ptr<core::ast::expr_type> parse_expr_type(parse_state& state);
static std::unique_ptr<core::ast::stmt_body> parse_stmt_body(parse_state& state);

static core::ast::p_expr PARSE_OPTIONAL_TYPE(parse_state& state) {
    if (state.now().type == core::token_type::COLON) {
        state.next();
        return parse_expr_type(state);
    }

    return std::make_unique<core::ast::expr_none>(state.now().selection);
}

static core::ast::p_expr binary_expression_left_associative(parse_state& state, const t_p_expression_function& lower, const t_operator_set& set) {
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

static core::ast::p_expr binary_expression_right_associative(parse_state& state, const t_p_expression_function& lower, const t_operator_set& set) {
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

static std::unique_ptr<core::ast::expr_type> parse_expr_type(parse_state& state) {
    core::ast::p_expr source = parse_scope_resolution(state);
    std::vector<std::unique_ptr<core::ast::expr_type>> argument_list = {};

    if (state.now().type == core::token_type::LARROW) {
        do {
            state.next();
            argument_list.emplace_back(parse_expr_type(state));
        } while (!state.at_eof() && state.now().type == core::token_type::COMMA);

        state.expect(core::token_type::RARROW, "Expected a closing arrow inside parameters list.");
    }

    return std::make_unique<core::ast::expr_type>(core::lisel(source->selection, state.now().selection), source, argument_list);
}

static std::unique_ptr<core::ast::expr_parameter> parse_expr_parameter(parse_state& state) {
    const core::token& start_token = state.now();
    
    std::vector<core::ast::parameter_qualifier> qualifiers = {};
    auto name = std::make_unique<core::ast::expr_identifier>(state.expect(core::token_type::IDENTIFIER, "Expected an identifier.").selection);

    core::ast::p_expr type = PARSE_OPTIONAL_TYPE(state);
    
    core::ast::p_expr default_value;

    if (state.now().type == core::token_type::EQUAL) {
        state.next();
        default_value = parse_expression(state);
    }
    else
        default_value = std::make_unique<core::ast::expr_none>(state.now().selection);

    return std::make_unique<core::ast::expr_parameter>(core::lisel(start_token.selection, state.now().selection), name, default_value, qualifiers, type);
}

static std::vector<std::unique_ptr<core::ast::expr_parameter>> parse_expr_parameter_list(parse_state& state) {
    std::vector<std::unique_ptr<core::ast::expr_parameter>> parameter_list;
    
    if (state.peek(1).type == core::token_type::RPAREN)
        return parameter_list;
    
    do {
        state.next();
        parameter_list.emplace_back(parse_expr_parameter(state));
    } while (!state.at_eof(), state.now().type == core::token_type::COMMA);
    
    state.expect(core::token_type::RPAREN, "Expected a closing parenthesis inside parameters list.");
    
    return parameter_list;
}

static std::unique_ptr<core::ast::expr_function> parse_expr_function(parse_state& state) {
    const core::token& start_token = state.now();
    std::vector<std::unique_ptr<core::ast::expr_parameter>> parameter_list = parse_expr_parameter_list(state);
    core::ast::p_expr return_type = PARSE_OPTIONAL_TYPE(state);

    core::ast::p_stmt body = parse_statement(state);

    return std::make_unique<core::ast::expr_function>(core::lisel(start_token.selection, state.now().selection), parameter_list, body, return_type); 
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

    state.process.add_log(core::lilog::log_level::ERROR, tok.selection, "Unexpected token.");

    return std::make_unique<core::ast::expr_invalid>(tok.selection);
}

static core::ast::p_expr parse_scope_resolution(parse_state& state) {
    return binary_expression_left_associative(state, &parse_primary_expression, set_scope_resolution);
}

static core::ast::p_expr parse_member_access(parse_state& state) {
    return binary_expression_left_associative(state, &parse_scope_resolution, set_member_access);
}

static core::ast::p_expr parse_expr_call(parse_state& state) {
    core::ast::p_expr expression = parse_member_access(state);

    if (state.now().type != core::token_type::LPAREN)
        return expression;

    std::vector<core::ast::p_expr> argument_list;

    if (state.peek(1).type != core::token_type::RPAREN)
        do {
            state.next();
            argument_list.emplace_back(parse_expression(state));
        } while (!state.at_eof() && state.now().type == core::token_type::COMMA);
    else
        state.next();

    state.expect(core::token_type::RPAREN, "Expected ')' after function call.");

    return std::make_unique<core::ast::expr_call>(core::lisel(expression->selection, state.now().selection), expression, argument_list);
}

static core::ast::p_expr parse_expr_unary(parse_state& state) {
    const core::token& start_token = state.now();

    if (set_unary_pre.find(state.now().type) != set_unary_pre.end()) {
        const core::token& opr = state.next();
        core::ast::p_expr operand = parse_expr_unary(state);
        return std::make_unique<core::ast::expr_unary>(core::lisel(start_token.selection, operand->selection), operand, opr, false);
    }

    core::ast::p_expr expression = parse_expr_call(state);

    if (set_unary_post.find(state.now().type) != set_unary_post.end()) {
        const core::token& opr = state.next();
        return std::make_unique<core::ast::expr_unary>(core::lisel(start_token.selection, opr.selection), expression, opr, true);
    }
    
    return expression;
}

static core::ast::p_expr parse_exponential(parse_state& state) {
    return binary_expression_right_associative(state, &parse_expr_unary, set_exponential);
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
    const core::token& start_token = state.next();
    core::ast::p_expr condition = parse_expression(state);
    core::ast::p_stmt consequent = parse_statement(state);
    core::ast::p_stmt alternate;

    if (state.now().type == core::token_type::ELSE) {
        state.next(); // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = std::make_unique<core::ast::stmt_none>(state.now().selection);

    return std::make_unique<core::ast::stmt_if>(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate);
}

static std::unique_ptr<core::ast::stmt_while> parse_stmt_while(parse_state& state) {
    const core::token& start_token = state.next();
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

    return std::make_unique<core::ast::stmt_while>(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate);
}

static std::unique_ptr<core::ast::stmt_body> parse_stmt_body(parse_state& state) {
    const core::token& brace_token = state.next();
    std::vector<core::ast::p_stmt> statement_list;

    while (!state.at_eof() && state.now().type != core::token_type::RBRACE) {
        statement_list.emplace_back(parse_statement(state));
    }

    if (state.at_eof())
        state.process.add_log(core::lilog::log_level::ERROR, state.now().selection, "Expected right brace, got EOF.");
    else
        state.next();
        
    return std::make_unique<core::ast::stmt_body>(core::lisel(brace_token.selection, state.now().selection), statement_list);
}

static std::unique_ptr<core::ast::stmt_declaration> parse_stmt_declaration(parse_state& state) {
    std::vector<core::ast::variable_qualifier> qualifiers = {};
    const core::token& start_token = state.next();
    
    core::ast::p_expr name = parse_scope_resolution(state);
    core::ast::p_expr type = PARSE_OPTIONAL_TYPE(state);

    core::ast::p_expr value;

    switch (state.now().type) {
        case core::token_type::LPAREN:
            value = parse_expr_function(state);
            break;
        case core::token_type::EQUAL:
            state.next();
            value = parse_expression(state);
            break;
        default:
            value = std::make_unique<core::ast::expr_none>(state.now().selection);
    }
    
    return std::make_unique<core::ast::stmt_declaration>(core::lisel(start_token.selection, state.now().selection), name, value, qualifiers, type);
}

static std::unique_ptr<core::ast::stmt_return> parse_stmt_return(parse_state& state) {
    const core::token& start_token = state.next();

    core::ast::p_expr expression;

    if (state.now().type == core::token_type::RBRACE)
        expression = std::make_unique<core::ast::expr_none>(state.now().selection);
    else
        expression = parse_expression(state);
    
    return std::make_unique<core::ast::stmt_return>(core::lisel(start_token.selection, expression->selection), expression);
}

static std::unique_ptr<core::ast::stmt_use> parse_stmt_use(parse_state& state) {
    const core::token& start_token = state.next();
    auto string = std::make_unique<core::ast::expr_literal>(state.expect(core::token_type::STRING, "Expected a string.").selection, core::ast::expr_literal::literal_type::STRING);

    return std::make_unique<core::ast::stmt_use>(core::lisel(start_token.selection, string->selection), string);
}

static core::ast::p_stmt parse_statement(parse_state& state) {
    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::IF: return parse_stmt_if(state);
        case core::token_type::WHILE: return parse_stmt_while(state);
        case core::token_type::LBRACE: return parse_stmt_body(state);
        case core::token_type::DEC: return parse_stmt_declaration(state);   // For variable qualifiers, we'll check for those and fallthrough.
        case core::token_type::RETURN: return parse_stmt_return(state);
        case core::token_type::USE: return parse_stmt_use(state);
        case core::token_type::BREAK: return std::make_unique<core::ast::stmt_break>(state.next().selection);
        case core::token_type::CONTINUE: return std::make_unique<core::ast::stmt_continue>(state.next().selection);
        default: {
            core::ast::p_expr expression = parse_expression(state);
            
            if (!expression->is_wrappable()) {
                state.process.add_log(core::lilog::log_level::ERROR, expression->selection, "Unexpected expression.");
                return std::make_unique<core::ast::stmt_invalid>(expression->selection);
            }
            
            return std::make_unique<core::ast::stmt_wrapper>(expression);
        }
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
