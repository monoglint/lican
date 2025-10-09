/*

====================================================

⚠️⚠️⚠️ MEMORY ALERT ⚠️⚠️⚠️
Vectors are moved into the ast nodes which means the the arena block isn't holding 100% of all of the ast data.
FIX!!!!

====================================================

TERMINOLOGY NOTE

All nodes are refered to as items unless
    - It is an expression that can not stand independently by design
    - It is only usable within function bodies

Expression nodes can stand independent in an item or statement wrapper if certain conditions are met (check ast.cc/is_expression_wrappable)

Statement nodes are items that can only exist in function bodies.

====================================================

TERMINOLOGY NOTE

Although this project works on an adjective-noun based naming convention, the AST and parser puts stmt, item, and expr before

the node type specification for better lookup. This might be changed in the future.

====================================================

STYLE NOTE

All functions in the parser are coded to assume that the first token has already been processed or taken care of.

====================================================

Inside of the parser, always invoke logs using state.log_and_pause errors if you want every other error
in the given statement to be ignored. This can help prevent cascading problems.

ctor can be an identifier

====================================================

*/

#include <unordered_set>
#include <unordered_map>

#include "core.hh"
#include "ast.hh"
#include "token.hh"

using namespace core::ast;

/*

====================================================

Constants
Here at Lican, we present half-softcoded files.

====================================================

*/

// Delimits arguments, tables, etc.
constexpr auto LIST_DELIMITER_TOKEN = core::token_type::COMMA;

constexpr auto L_EXPR_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_EXPR_DELIMITER_TOKEN = core::token_type::RPAREN;

// Used by function parameters and arguments.
constexpr auto L_FUNC_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_FUNC_DELIMITER_TOKEN = core::token_type::RPAREN;

// Used by type parameters and arguments.
constexpr auto L_TEMPLATE_DELIMITER_TOKEN = core::token_type::LSQUARE;
constexpr auto R_TEMPLATE_DELIMITER_TOKEN = core::token_type::RSQUARE;

// Like C-style braces.
constexpr auto L_BODY_DELIMITER_TOKEN = core::token_type::LBRACE;
constexpr auto R_BODY_DELIMITER_TOKEN = core::token_type::RBRACE;

constexpr auto TYPE_DENOTER_TOKEN = core::token_type::COLON;
constexpr auto TYPE_POINTER_TOKEN = core::token_type::AT;
constexpr auto TYPE_LVALUE_REFERENCE_TOKEN = core::token_type::AMPERSAND;
constexpr auto TYPE_RVALUE_REFERENCE_TOKEN = core::token_type::DOUBLE_AMPERSAND;

// dec x = 5
constexpr auto ASSIGNMENT_TOKEN = core::token_type::EQUAL;

// x ? 5 : 2
constexpr auto TERNARY_CONDITION_TOKEN = core::token_type::QUESTION;
constexpr auto TERNARY_ELSE_TOKEN = core::token_type::COLON;

constexpr auto INITIALIZER_LIST_START_TOKEN = core::token_type::RPTR;
constexpr auto L_INITIALIZER_SET_DELIMITER_TOKEN = core::token_type::LPAREN;
constexpr auto R_INITIALIZER_SET_DELIMITER_TOKEN = core::token_type::RPAREN;

// Forward declarations
struct parse_state;

static t_node_id parse_expression(parse_state& state);
static t_node_id parse_scope_resolution(parse_state& state);
static t_node_id parse_statement(parse_state& state);
static t_node_id parse_item(parse_state& state);
static t_node_id parse_variant_declaration(parse_state& state, const bool local_declaration);

static t_node_id parse_expr_type(parse_state& state);

template <typename T_NODE, typename PARSE_FUNC>
static t_node_id parse_item_body(parse_state& state, PARSE_FUNC& parse_func);

/*

====================================================

Basic sets for the binary expression segment of the parser

====================================================

*/

using t_token_set = std::unordered_set<core::token_type>;

static const t_token_set binary_scope_resolution_set = {
    core::token_type::DOUBLE_DOT,
};

static const t_token_set binary_member_access_set = {
    core::token_type::DOT,
};

static const t_token_set unary_post_set = {
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_token_set unary_pre_set = {
    core::token_type::MINUS, // negate
    core::token_type::BANG, // not
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS,
    core::token_type::AT, // address of
    core::token_type::ASTERISK, // derference
};

static const t_token_set binary_exponential_set = {
    core::token_type::CARET,
};

static const t_token_set binary_multiplicative_set = {
    core::token_type::ASTERISK,
    core::token_type::SLASH,
    core::token_type::PERCENT,
};

static const t_token_set binary_additive_set = {
    core::token_type::PLUS,
    core::token_type::MINUS,
};

static const t_token_set binary_numeric_comparison_set = {
    core::token_type::LARROW,
    core::token_type::LESS_EQUAL,
    core::token_type::RARROW,
    core::token_type::GREATER_EQUAL,
};

static const t_token_set binary_direct_comparison_set = {
    core::token_type::DOUBLE_EQUAL,
    core::token_type::BANG_EQUAL,
};

static const t_token_set binary_and_set = {
    core::token_type::DOUBLE_AMPERSAND,
};

static const t_token_set binary_or_set = {
    core::token_type::DOUBLE_PIPE,
};

static const t_token_set binary_assignment_set = {
    core::token_type::EQUAL,
    core::token_type::PLUS_EQUAL,
    core::token_type::MINUS_EQUAL,
    core::token_type::ASTERISK_EQUAL,
    core::token_type::SLASH_EQUAL,
    core::token_type::PERCENT_EQUAL,
    core::token_type::CARET_EQUAL,
};

struct parse_state {
    parse_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), file(process.file_list[file_id]), token_list(std::any_cast<const std::vector<core::token>&>(process.file_list[file_id].dump_token_list)) {}

    core::liprocess& process;

    const core::t_file_id file_id;
    core::liprocess::lifile& file;

    const std::vector<core::token>& token_list; // Ref to process property

    ast_arena arena;

    core::t_pos pos = 0;

    // When true, all logs will be set as cascaded. They still get sent to the core, but with lower priority.
    bool f_pause_errors = false;

    inline const core::token& now() const {
        return token_list[pos];
    }

    inline const core::token& consume() {
        if (at_eof())
            return now();

        return token_list[pos++];
    }
   
    inline const core::token& peek(const core::t_pos amount = 1) const {
        if (!is_peek_safe(amount))
            return token_list[token_list.size() - 1];
       
        return token_list[pos + amount];
    }
   
    inline bool is_peek_safe(const core::t_pos amount = 1) const {
        return pos + amount < token_list.size() - 1;
    }

    // Accounts for EOF token
    inline bool at_eof() const {
        return pos >= token_list.size() - 1;
    }

    inline const core::token& expect(const core::token_type type, const std::string& error_message = "[No Info]") {
        const core::token& now = consume();
        if (now.type != type)
            log_and_pause_errors(core::lilog::log_level::ERROR, now.selection, "Unexpected token - " + error_message);
       
        // We expect to return an incorrect token.
        // Since the parser handles tokens by reference, it is not a good idea to make new temporary ones.
        return now;
    }

    inline void log_and_pause_errors(const core::lilog::log_level log_level, const core::lisel& selection, const std::string& message) {
        if (f_pause_errors)
            return;

        process.add_log(log_level, selection, message);

        if (!process.config._show_cascading_logs)
            f_pause_errors = true;        
    }
};

static t_node_id parse_optional_type(parse_state& state) {
    if (state.now().type == TYPE_DENOTER_TOKEN) {
        state.pos++;
        return parse_expr_type(state);
    }

    return state.arena.insert(expr_none(state.now().selection));
}

template <bool RIGHT_ASSOCIATION, typename FUNC>
static t_node_id binary_expression_associative(parse_state& state, const FUNC& lower, const t_token_set& set) {
    t_node_id left = lower(state);

    // Braces are required for outer if-constexpr so the else doesn't get captured by the inner if statement.
    if constexpr (RIGHT_ASSOCIATION) {
        if (!state.at_eof() && set.find(state.now().type) != set.end()) {
            const core::token& opr = state.consume();
            const t_node_id right = binary_expression_associative<true>(state, lower, set);
        
            return state.arena.insert(expr_binary(
                core::lisel(state.arena.get_base_ptr(left)->selection, state.arena.get_base_ptr(right)->selection),
                left,
                right,
                opr
            ));
        }
    }
    else
        while (!state.at_eof() && set.find(state.now().type) != set.end()) {
            const core::token& opr = state.consume();
            const t_node_id right = lower(state);

            left = state.arena.insert(expr_binary(
                core::lisel(state.arena.get_base_ptr(left)->selection, state.arena.get_base_ptr(right)->selection),
                left,
                right,
                opr
            ));
        }

    return left;
}

template <bool IS_OPTIONAL, bool USE_LIST_DELIMITER, typename FUNC>
static t_node_list parse_list(parse_state& state, FUNC func, const core::token_type left_delim, const core::token_type right_delim) {
    if (state.now().type != left_delim)
        if constexpr (IS_OPTIONAL) 
            return {};
        else
            state.log_and_pause_errors(core::lilog::log_level::ERROR, state.now().selection, "Expected an opening delimiter.");

    if (state.peek(1).type == right_delim) {
        state.pos += 2;
        return {};    
    }

    t_node_list list = {};

    if constexpr (USE_LIST_DELIMITER) {
        do {
            state.pos++;
            list.push_back(func(state));
        } while (!state.at_eof() && state.now().type == LIST_DELIMITER_TOKEN);
        state.expect(right_delim, "Expected a closing delimiter.");

        return list;
    }

    state.pos++;
    do {
        list.push_back(func(state));
    } while (!state.at_eof() && state.now().type != right_delim);
    state.pos++;

    return list;
}

static t_node_id parse_expr_type(parse_state& state) {
    const bool is_const = state.now().type == core::token_type::CONST;
    if (is_const) state.pos++;

    const bool is_pointer = state.now().type == TYPE_POINTER_TOKEN;
    if (is_pointer) state.pos++;

    const t_node_id source = parse_scope_resolution(state);

    t_node_list argument_list = parse_list<true, true>(state, parse_expr_type, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);

    expr_type::e_reference_type reference_type;
    switch (state.now().type) {
        case TYPE_LVALUE_REFERENCE_TOKEN:
            reference_type = expr_type::e_reference_type::LVALUE;
            state.pos++;
            break;
        case TYPE_RVALUE_REFERENCE_TOKEN:
            reference_type = expr_type::e_reference_type::RVALUE;
            state.pos++;
            break;
        default:
            reference_type = expr_type::e_reference_type::NONE;
    }

    return state.arena.insert(expr_type(core::lisel(state.arena.get_base_ptr(source)->selection, state.now().selection), source, std::move(argument_list), is_const, is_pointer, reference_type));
}

static t_node_id parse_expr_parameter(parse_state& state) {
    const core::token& start_token = state.now();
   
    const t_node_id name = state.arena.insert(expr_identifier(state.expect(core::token_type::IDENTIFIER, "Expected an identifier.").selection));
    const t_node_id value_type = parse_optional_type(state);
   
    t_node_id default_value;

    if (state.now().type == ASSIGNMENT_TOKEN) {
        state.pos++;
        default_value = parse_expression(state);
    }
    else
        default_value = state.arena.insert(expr_none(state.now().selection));

    return state.arena.insert(expr_parameter(core::lisel(start_token.selection, state.now().selection), name, default_value, value_type));
}

template <bool IS_OPTIONAL>
static t_node_id parse_expr_identifier(parse_state& state) {
    if constexpr (IS_OPTIONAL)
        if (state.now().type == core::token_type::IDENTIFIER)
            return state.arena.insert(expr_identifier(state.consume().selection));
        else
            return state.arena.insert(expr_none(state.now().selection));
    
    const core::token& token = state.expect(core::token_type::IDENTIFIER, "Expected an identifier.");
    
    if (token.type != core::token_type::IDENTIFIER)
        return state.arena.insert(expr_invalid(token.selection));    
    
    return state.arena.insert(expr_identifier(token.selection));
}

static t_node_id parse_expr_int_literal(parse_state& state) {
    return state.arena.insert(expr_literal(state.expect(core::token_type::INT, "Expected an integer.").selection, expr_literal::e_literal_type::INT));
}

static t_node_id parse_expr_function(parse_state& state) {
    const core::token& start_token = state.now();

    t_node_list template_parameter_list = parse_list<true, true>(state, parse_expr_identifier<false>, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);
    t_node_list parameter_list = parse_list<false, true>(state, parse_expr_parameter, L_FUNC_DELIMITER_TOKEN, R_FUNC_DELIMITER_TOKEN);
    const t_node_id return_type = parse_optional_type(state);

    const t_node_id body = parse_statement(state);

    return state.arena.insert(expr_function(core::lisel(start_token.selection, state.now().selection), std::move(template_parameter_list), std::move(parameter_list), body, return_type));
}

#define CASE_LITERAL(type) \
    case core::token_type::type: \
        return state.arena.insert(expr_literal(state.consume().selection, expr_literal::e_literal_type::type));

static t_node_id parse_primary_expression(parse_state& state) {
    switch (state.now().type) {
        case core::token_type::IDENTIFIER:
            return state.arena.insert(expr_identifier(state.consume().selection));
    
        CASE_LITERAL(INT)
        CASE_LITERAL(FLOAT)
        CASE_LITERAL(STRING)
        CASE_LITERAL(CHAR)
        CASE_LITERAL(NIL)

        case core::token_type::FALSE: [[fallthrough]];
        case core::token_type::TRUE:
            return state.arena.insert(expr_literal(state.consume().selection, expr_literal::e_literal_type::BOOL));
            
        case core::token_type::DEC:
            return parse_variant_declaration(state, true);
        case L_EXPR_DELIMITER_TOKEN: {
            state.pos++;
            t_node_id expr = parse_expression(state);
            state.expect(R_EXPR_DELIMITER_TOKEN, "Expected closing delimiter after expression.");
            return expr;
        }
        default:
            break;
    }

    state.log_and_pause_errors(core::lilog::log_level::ERROR, state.now().selection, "Unexpected token.");

    return state.arena.insert(expr_invalid(state.consume().selection));
}

#undef CASE_LITERAL

static t_node_id parse_scope_resolution(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_primary_expression, binary_scope_resolution_set);
}

static t_node_id parse_member_access(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_scope_resolution, binary_member_access_set);
}

static t_node_id parse_expr_call(parse_state& state) {
    t_node_id expression;

    // Allow 'ctor' to be called. This should only be done in the context of constructor delegation.
    if (state.now().type == core::token_type::CTOR)
        expression = state.arena.insert(expr_identifier(state.consume().selection));
    else {
        expression = parse_member_access(state);
        const node_type expr_type = state.arena.get_base_ptr(expression)->type;

        if (expr_type != node_type::EXPR_BINARY && expr_type != node_type::EXPR_IDENTIFIER || state.now().type != L_FUNC_DELIMITER_TOKEN && state.now().type != L_TEMPLATE_DELIMITER_TOKEN)
            return expression;
    }

    t_node_list type_argument_list = parse_list<true, true>(state, parse_expr_type, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);
    t_node_list argument_list = parse_list<false, true>(state, parse_expression, L_FUNC_DELIMITER_TOKEN, R_FUNC_DELIMITER_TOKEN);

    return state.arena.insert(expr_call(core::lisel(state.arena.get_base_ptr(expression)->selection, state.now().selection), expression, std::move(type_argument_list), std::move(argument_list)));
}

static t_node_id parse_expr_unary(parse_state& state) {
    const core::token& start_token = state.now();

    if (unary_pre_set.find(state.now().type) != unary_pre_set.end()) {
        const core::token& opr = state.consume();
        t_node_id operand = parse_expr_unary(state);
        return state.arena.insert(expr_unary(core::lisel(start_token.selection, state.arena.get_base_ptr(operand)->selection), operand, opr, false));
    }

    const t_node_id expression = parse_expr_call(state);

    if (unary_post_set.find(state.now().type) != unary_post_set.end()) {
        const core::token& opr = state.consume();
        return state.arena.insert(expr_unary(core::lisel(start_token.selection, opr.selection), expression, opr, true));
    }
   
    return expression;
}

static t_node_id parse_exponential(parse_state& state) {
    return binary_expression_associative<true>(state, &parse_expr_unary, binary_exponential_set);
}

static t_node_id parse_multiplicative(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_exponential, binary_multiplicative_set);
}

static t_node_id parse_additive(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_multiplicative, binary_additive_set);
}

static t_node_id parse_numeric_comparison(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_additive, binary_numeric_comparison_set);
}

static t_node_id parse_direct_comparison(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_numeric_comparison, binary_direct_comparison_set);
}

static t_node_id parse_and(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_direct_comparison, binary_and_set);
}

static t_node_id parse_or(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_and, binary_or_set);
}

static t_node_id parse_expr_ternary(parse_state& state) {
    const t_node_id first = parse_or(state);
    if (state.now().type != TERNARY_CONDITION_TOKEN)
        return first;
   
    state.pos++;
    const t_node_id second = parse_expression(state);
    state.expect(TERNARY_ELSE_TOKEN, "Expected a ternary-else-symbol.");
    const t_node_id third = parse_expression(state);
   
    return state.arena.insert(expr_ternary(core::lisel(state.arena.get_base_ptr(first)->selection, state.arena.get_base_ptr(third)->selection), first, second, third));
}

static t_node_id parse_assignment(parse_state& state) {
    return binary_expression_associative<false>(state, &parse_expr_ternary, binary_assignment_set);
}

// Entry point to pratt parser design
static t_node_id parse_expression(parse_state& state) {
    return parse_assignment(state);
}

static t_node_id parse_stmt_if(parse_state& state) {
    const core::token& start_token = state.consume();
    const t_node_id condition = parse_expression(state);
    const t_node_id consequent = parse_statement(state);
    t_node_id alternate;

    if (state.now().type == core::token_type::ELSE) {
        state.pos++; // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = state.arena.insert(stmt_none(state.now().selection));

    return state.arena.insert(stmt_if(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate));
}

static t_node_id parse_stmt_while(parse_state& state) {
    const core::token& start_token = state.consume();
    const t_node_id condition = parse_expression(state);
    const t_node_id consequent = parse_statement(state);
    t_node_id alternate;

    // In while loops, else's run if the condition fails on the first time.
    if (state.now().type == core::token_type::ELSE) {
        state.pos++; // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = state.arena.insert(stmt_none(state.now().selection));

    return state.arena.insert(stmt_while(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate));
}

template <typename T_NODE, typename PARSE_FUNC>
static t_node_id parse_item_body(parse_state& state, PARSE_FUNC& parse_func) {
    const core::token& brace_token = state.now();
    t_node_list item_list = parse_list<false, false>(state, parse_func, L_BODY_DELIMITER_TOKEN, R_BODY_DELIMITER_TOKEN);
       
    return state.arena.insert(T_NODE(core::lisel(brace_token.selection, state.now().selection), std::move(item_list)));
}

static t_node_id parse_stmt_return(parse_state& state) {
    const core::token& start_token = state.consume();

    t_node_id expression;

    if (state.now().type == R_BODY_DELIMITER_TOKEN)
        expression = state.arena.insert(expr_none(state.now().selection));
    else
        expression = parse_expression(state);
   
    return state.arena.insert(stmt_return(core::lisel(start_token.selection, state.arena.get_base_ptr(expression)->selection), expression));
}

static t_node_id parse_item_use(parse_state& state) {
    const core::token& start_token = state.consume();
    const core::token& value_token = state.expect(core::token_type::STRING, "Expected a string.");

    const t_node_id value_node = state.arena.insert(expr_literal(value_token.selection, expr_literal::e_literal_type::STRING));

    return state.arena.insert(item_use(core::lisel(start_token.selection, state.arena.get_base_ptr(value_node)->selection), value_node));
}

static t_node_id parse_item_module(parse_state& state) {
    const core::token& start_token = state.consume();
    const core::token& value_token = state.expect(core::token_type::IDENTIFIER, "Expected an identifier.");

    const t_node_id name_node = state.arena.insert(expr_identifier(value_token.selection));
    const t_node_id content = parse_item(state);
   
    return state.arena.insert(item_module(core::lisel(start_token.selection, state.arena.get_base_ptr(content)->selection), name_node, content));
}

static t_node_id parse_variant_declaration(parse_state& state, const bool local_declaration) {
    const core::token& start_token = state.consume();
   
    const t_node_id name = parse_scope_resolution(state);
    const t_node_id value_type = parse_optional_type(state);

    t_node_id value;

    switch (state.now().type) {
        case L_TEMPLATE_DELIMITER_TOKEN: // for potential type parameters
        case L_FUNC_DELIMITER_TOKEN:
            if (!local_declaration) {
                value = parse_expr_function(state);
                break;
            }
            value = state.arena.insert(expr_invalid(state.now().selection));
            state.log_and_pause_errors(core::lilog::log_level::ERROR, state.consume().selection, "Functions can not be declared in function bodies. Declare a closure instead.");
            value = state.arena.insert(expr_invalid(state.now().selection));
            break;
        case ASSIGNMENT_TOKEN:
            state.pos++;
            value = parse_expression(state);
            break;
        default:
            value = state.arena.insert(expr_none(state.now().selection));
    }

    return state.arena.insert(variant_declaration(core::lisel(start_token.selection, state.now().selection), name, value, value_type));
}

static t_node_id parse_item_type_declaration(parse_state& state) {
    const core::token& start_token = state.consume();
   
    const t_node_id name = parse_scope_resolution(state);
    t_node_list template_parameter_list = parse_list<true, true>(state, parse_expr_identifier<false>, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);

    state.expect(ASSIGNMENT_TOKEN, "Expected an assignment symbol.");

    const t_node_id type_value = parse_expr_type(state);

    return state.arena.insert(item_type_declaration(core::lisel(start_token.selection, state.now().selection), name, type_value, std::move(template_parameter_list)));
}

static t_node_id parse_expr_enum_set(parse_state& state) {
    const t_node_id name = parse_expr_identifier<false>(state);
    t_node_id value;

    if (state.now().type == ASSIGNMENT_TOKEN) {
        state.pos++;
        value = parse_expr_int_literal(state);
    }
    else
        value = state.arena.insert(expr_none(state.now().selection));

    return state.arena.insert(expr_enum_set(core::lisel(state.arena.get_base_ptr(name)->selection, state.now().selection), name, value));
}

static t_node_id parse_item_enum(parse_state& state) {
    const core::token& start_token = state.consume();

    const t_node_id name = parse_scope_resolution(state);
    state.expect(ASSIGNMENT_TOKEN, "Expected an assignment symbol.");

    t_node_list set_list = parse_list<false, false>(state, parse_expr_enum_set, L_BODY_DELIMITER_TOKEN, R_BODY_DELIMITER_TOKEN);

    return state.arena.insert(item_enum(core::lisel(start_token.selection, state.now().selection), name, std::move(set_list)));
}

static t_node_id parse_expr_operator(parse_state& state) {
    const core::token& start_token = state.consume();

    const core::token& opr_token = state.consume();

    // !FIX
    // Ensure the opr token is double checked to be a valid operator.

    const t_node_id function = parse_expr_function(state);

    const bool is_const = state.now().type == core::token_type::CONST;
    if (is_const)
        state.pos++;

    return state.arena.insert(expr_operator(core::lisel(start_token.selection, state.now().selection), opr_token.type, function, is_const));
}

// There will never be a condition in which this is not optional which is why there is no template.
static t_node_list parse_initializer_list(parse_state& state) {
    const core::token& start_token = state.now();

    if (start_token.type != INITIALIZER_LIST_START_TOKEN)
        return {};

    t_node_list initializer_list = {};

    do {
        state.pos++;
        const t_node_id property_name = parse_expr_identifier<false>(state);
        state.expect(L_INITIALIZER_SET_DELIMITER_TOKEN, "Expected a left delimiter.");
        const t_node_id value = parse_expression(state);
        state.expect(R_INITIALIZER_SET_DELIMITER_TOKEN, "Expected a right delimiter.");

        initializer_list.emplace_back(
            state.arena.insert(
                expr_initializer_set(
                    core::lisel(state.arena.get_base_ptr(property_name)->selection, state.now().selection),
                    property_name,
                    value
                )
            )
        );
    } while (!state.at_eof() && state.now().type == LIST_DELIMITER_TOKEN);

    return initializer_list;
}

// function_symbol, initializer_list
static std::pair<t_node_id, t_node_list> parse_constructor_function(parse_state& state) {
    const core::token& start_token = state.now();

    t_node_list template_parameter_list = parse_list<true, true>(state, parse_expr_identifier<false>, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);
    t_node_list parameter_list = parse_list<false, true>(state, parse_expr_parameter, L_FUNC_DELIMITER_TOKEN, R_FUNC_DELIMITER_TOKEN);
    const t_node_id return_type = parse_optional_type(state);

    t_node_list initializer_list = parse_initializer_list(state);

    const t_node_id body = parse_statement(state);

    return std::make_pair(
        state.arena.insert(
            expr_function(
                core::lisel(start_token.selection, state.now().selection), 
                std::move(template_parameter_list), 
                std::move(parameter_list), 
                body, 
                return_type
            )
        ),
        std::move(initializer_list)
    );
}

static t_node_id parse_expr_constructor(parse_state& state) {
    const core::token& start_token = state.consume();
    const t_node_id name = parse_expr_identifier<true>(state);

    auto pair = parse_constructor_function(state);
    
    return state.arena.insert(expr_constructor(core::lisel(start_token.selection, state.now().selection), name, pair.first, std::move(pair.second)));
}

static t_node_id parse_expr_destructor(parse_state& state) {
    const core::token& start_token = state.consume();

    const t_node_id body = parse_statement(state);

    return state.arena.insert(expr_destructor(core::lisel(start_token.selection, state.arena.get_base_ptr(body)->selection), body));
}

static t_node_id parse_expr_struct_member(parse_state& state) {
    switch (state.now().type) {
        case core::token_type::CTOR:
            return parse_expr_constructor(state);
        case core::token_type::DTOR:
            return parse_expr_destructor(state);
        case core::token_type::OPR:
            return parse_expr_operator(state);
    }

    const core::token& start_token = state.now();

    bool is_private = start_token.type == core::token_type::PRIV;

    if (is_private)
        state.pos++;

    const t_node_id name = parse_expr_identifier<false>(state);

    if (state.arena.get_base_ptr(name)->type == node_type::ITEM_INVALID)
        return name;

    switch (state.now().type) {
        case L_TEMPLATE_DELIMITER_TOKEN:
        case L_FUNC_DELIMITER_TOKEN: {
            const t_node_id function = parse_expr_function(state);

            const bool is_const = state.now().type == core::token_type::CONST;
            if (is_const)
                state.pos++;
                    
            return state.arena.insert(expr_method(core::lisel(start_token.selection, state.now().selection), name, function, is_private, is_const));
        }
        case TYPE_DENOTER_TOKEN: {
            state.pos++;

            const t_node_id value_type = parse_expr_type(state);

            t_node_id default_value;
            if (state.now().type == ASSIGNMENT_TOKEN) {
                state.pos++;
                default_value = parse_expression(state);
            }
            else
                default_value = state.arena.insert(expr_none(state.now().selection));

            return state.arena.insert(expr_property(core::lisel(start_token.selection, state.now().selection), name, value_type, default_value, is_private));
        }
        case ASSIGNMENT_TOKEN: {
            state.pos++;

            const t_node_id default_value = parse_expression(state);

            return state.arena.insert(
                expr_property(
                    core::lisel(start_token.selection, state.now().selection), 
                    name, 
                    state.arena.insert(expr_none(state.now().selection)), 
                    default_value, 
                    is_private
                )
            );
        }
        default:
            state.log_and_pause_errors(
                core::lilog::log_level::ERROR,
                state.now().selection,
                "Unexpected token. Either set \"" + state.process.sub_source_code(state.arena.get_base_ptr(name)->selection) + "\" to a property or method."
            );
    }

    return state.arena.insert(expr_invalid(state.now().selection));
}

static t_node_id parse_item_struct(parse_state& state) {
    const core::token& start_token = state.consume();

    const t_node_id name = parse_scope_resolution(state);

    t_node_list template_parameter_list = parse_list<true, true>(state, parse_expr_identifier<false>, L_TEMPLATE_DELIMITER_TOKEN, R_TEMPLATE_DELIMITER_TOKEN);
    t_node_list member_list = parse_list<false, false>(state, parse_expr_struct_member, L_BODY_DELIMITER_TOKEN, R_BODY_DELIMITER_TOKEN);

    return state.arena.insert(item_struct_declaration(core::lisel(start_token.selection, state.now().selection), name, std::move(template_parameter_list), std::move(member_list)));
}

// Find statements expected in a module or a struct.
static t_node_id parse_item(parse_state& state) {
    state.f_pause_errors = false;

    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::USE: return parse_item_use(state);
        case core::token_type::MODULE: return parse_item_module(state);
        case core::token_type::DEC: return parse_variant_declaration(state, false);
        case core::token_type::TYPEDEC: return parse_item_type_declaration(state);
        case core::token_type::ENUM: return parse_item_enum(state);
        case core::token_type::STRUCT: return parse_item_struct(state);
        case L_BODY_DELIMITER_TOKEN: return parse_item_body<item_body>(state, parse_item);
        default: {
            const node* statement = state.arena.get_base_ptr(parse_statement(state));
            state.log_and_pause_errors(core::lilog::log_level::ERROR, statement->selection, "The given item can only be used in a function body.");
            return state.arena.insert(item_invalid(statement->selection));
        }
    }
}

// Find statements expected in function bodies.
static t_node_id parse_statement(parse_state& state) {
    state.f_pause_errors = false;

    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::IF: return parse_stmt_if(state);
        case core::token_type::WHILE: return parse_stmt_while(state);
        case L_BODY_DELIMITER_TOKEN: return parse_item_body<item_body>(state, parse_statement);
        case core::token_type::RETURN: return parse_stmt_return(state);
        case core::token_type::TYPEDEC: return parse_item_type_declaration(state);
        case core::token_type::BREAK: return state.arena.insert(stmt_break(state.consume().selection));
        case core::token_type::CONTINUE: return state.arena.insert(stmt_continue(state.consume().selection));

        // Capture this for items that are not statement compatible
        case core::token_type::USE:
        case core::token_type::MODULE:
            state.log_and_pause_errors(core::lilog::log_level::ERROR, tok.selection, "The given item can not be used in a function body.");
            return state.arena.insert(stmt_invalid(state.consume().selection));

        // Reserve the default case for expression wrapping.
        default: {
            t_node_id expr_id = parse_expression(state);
           
            if (!state.arena.is_expression_wrappable(expr_id)) {
                const node* as_node = state.arena.get_base_ptr(expr_id);
                state.log_and_pause_errors(core::lilog::log_level::ERROR, as_node->selection, "Unexpected expression.");
                return state.arena.insert(stmt_invalid(as_node->selection));
            }
           
            return expr_id;
        }
    }
}

bool core::frontend::parse(core::liprocess& process, const core::t_file_id file_id) {
    parse_state state(process, file_id);
   
    state.arena.insert(ast_root());
   
    while (!state.at_eof()) {
        auto result = parse_item(state);

        state.arena.get_as<ast_root>(0).item_list.push_back(std::move(result));
    }

    state.file.dump_ast_arena = std::any(std::move(state.arena));

    return true;
}