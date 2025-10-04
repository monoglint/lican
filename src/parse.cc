/*

====================================================

TERMINOLOGY NOTE

All nodes are refered to as items unless
    - It is an expression that can not stand independently by design
    - It is only usable within function bodies

Expression nodes can stand independent in an item or statement wrapper if certain conditions are met (check ast.cpp/is_expression_wrappable)

Statement nodes are items that can only exist in function bodies.

====================================================

Inside of the parser, always invoke logs using state.log_and_pause errors if you want every other error
in the given statement to be ignored. This can help prevent cascading problems.

====================================================

*/

#include <unordered_set>
#include <unordered_map>

#include "core.hh"
#include "ast.hh"
#include "token.hh"

// Constants

const auto LIST_DELIMITER = core::token_type::COMMA;

const auto L_EXPR_DELIMITER = core::token_type::LPAREN;
const auto R_EXPR_DELIMITER = core::token_type::RPAREN;

// Used by function parameters and arguments.
const auto L_FUNC_DELIMITER = core::token_type::LPAREN;
const auto R_FUNC_DELIMITER = core::token_type::RPAREN;

// Used by type parameters and arguments.
const auto L_TYPE_DELIMITER = core::token_type::LSQUARE;
const auto R_TYPE_DELIMITER = core::token_type::RSQUARE;

// Like C-style braces.
const auto L_BODY_DELIMITER = core::token_type::LBRACE;
const auto R_BODY_DELIMITER = core::token_type::RBRACE;

const auto TYPE_DENOTER = core::token_type::COLON;

// Forward declarations
struct parse_state;

static core::ast::t_node_id parse_expression(parse_state& state);
static core::ast::t_node_id parse_scope_resolution(parse_state& state);
static core::ast::t_node_id parse_statement(parse_state& state);
static core::ast::t_node_id parse_item(parse_state& state);
static core::ast::t_node_id parse_variant_declaration(parse_state& state, const bool local_declaration);

static core::ast::t_node_id parse_expr_type(parse_state& state);

template <typename T_NODE, typename PARSE_FUNC>
static core::ast::t_node_id parse_item_body(parse_state& state, PARSE_FUNC& parse_func);

/*

====================================================

Basic sets for the binary expression segment of the parser

====================================================

*/

using t_token_set = std::unordered_set<core::token_type>;

static const t_token_set binary_scope_resolution_set = {
    core::token_type::DOUBLE_COLON
};

static const t_token_set binary_member_access_set = {
    core::token_type::DOT
};

static const t_token_set unary_post_set = {
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_token_set unary_pre_set = {
    core::token_type::MINUS,
    core::token_type::BANG,
    core::token_type::DOUBLE_PLUS,
    core::token_type::DOUBLE_MINUS
};

static const t_token_set binary_exponential_set = {
    core::token_type::CARET
};

static const t_token_set binary_multiplicative_set = {
    core::token_type::ASTERISK,
    core::token_type::SLASH,
    core::token_type::PERCENT
};

static const t_token_set binary_additive_set = {
    core::token_type::PLUS,
    core::token_type::MINUS
};

static const t_token_set binary_numeric_comparison_set = {
    core::token_type::LARROW,
    core::token_type::LESS_EQUAL,
    core::token_type::RARROW,
    core::token_type::GREATER_EQUAL
};

static const t_token_set binary_direct_comparison_set = {
    core::token_type::DOUBLE_EQUAL,
    core::token_type::BANG_EQUAL
};

static const t_token_set binary_and_set = {
    core::token_type::DOUBLE_AMPERSAND
};

static const t_token_set binary_or_set = {
    core::token_type::DOUBLE_PIPE
};

static const t_token_set binary_assignment_set = {
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

    core::ast::ast_arena arena;

    core::t_pos pos = 0;

    // When true, all logs will be set as cascaded. They still get sent to the core, but with lower priority.
    bool f_pause_errors = false;

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

    // Accounts for EOF token
    inline bool at_eof() const {
        return pos >= token_list.size() - 1;
    }

    inline const core::token& expect(const core::token_type type, const std::string& error_message = "[No Info]") {
        const core::token& now = next();
        if (now.type != type)
            log_and_pause_errors(core::lilog::log_level::ERROR, now.selection, "Unexpected token: " + error_message);
       
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

static core::ast::t_node_id parse_optional_type(parse_state& state) {
    if (state.now().type == TYPE_DENOTER) {
        state.pos++;
        return parse_expr_type(state);
    }

    return state.arena.insert(core::ast::expr_none(state.now().selection));
}

template <typename FUNC>
static core::ast::t_node_id binary_expression_left_associative(parse_state& state, const FUNC& lower, const t_token_set& set) {
    core::ast::t_node_id left = lower(state);

    while (!state.at_eof() && set.find(state.now().type) != set.end()) {
        const core::token& opr = state.next();
        const core::ast::t_node_id right = lower(state);

        left = state.arena.insert(core::ast::expr_binary(
            core::lisel(state.arena.get_base_ptr(left)->selection, state.arena.get_base_ptr(right)->selection),
            left,
            right,
            opr
        ));
    }

    return left;
}

template <typename FUNC>
static core::ast::t_node_id binary_expression_right_associative(parse_state& state, const FUNC& lower, const t_token_set& set) {
    core::ast::t_node_id left = lower(state);

    if (!state.at_eof() && set.find(state.now().type) != set.end()) {
        const core::token& opr = state.next();

        // recurse on *binary_expression* instead of just *lower*
        const core::ast::t_node_id right = binary_expression_right_associative(state, lower, set);
       
        return state.arena.insert(core::ast::expr_binary(
            core::lisel(state.arena.get_base_ptr(left)->selection, state.arena.get_base_ptr(right)->selection),
            left,
            right,
            opr
        ));
    }

    return left;
}

// FUNC can be used to make a list of identifiers or a list of type expressions.
template <bool IS_OPTIONAL, bool USE_LIST_DELIMITER, typename FUNC>
static core::ast::t_node_list parse_list(parse_state& state, FUNC func, const core::token_type left_delim, const core::token_type right_delim) {
    if (state.now().type != left_delim)
        if constexpr (IS_OPTIONAL) return {};
        else state.log_and_pause_errors(core::lilog::log_level::ERROR, state.now().selection, "Expected an opening delimiter.");

    if (state.peek(1).type == right_delim) {
        state.pos += 2;
        return {};    
    }

    core::ast::t_node_list list = {};

    if constexpr (USE_LIST_DELIMITER) {
        do {
            state.pos++;
            list.push_back(func(state));
        } while (!state.at_eof() && state.now().type == LIST_DELIMITER);
        state.expect(right_delim, "Expected a closing delimiter.");
    }
    else {
        state.pos++;
        do {
            list.push_back(func(state));
        } while (!state.at_eof() && state.now().type != right_delim);
        state.pos++;
    }

    return list;
}

static core::ast::t_node_id parse_expr_type(parse_state& state) {
    const bool is_mutable = state.now().type == core::token_type::POUND;
    if (is_mutable) state.next();

    const bool is_pointer = state.now().type == core::token_type::AT;
    if (is_pointer) state.next();

    const core::ast::t_node_id source = parse_scope_resolution(state);

    core::ast::t_node_list argument_list = parse_list<true, true>(state, parse_expr_type, L_TYPE_DELIMITER, R_TYPE_DELIMITER);

    return state.arena.insert(core::ast::expr_type(core::lisel(state.arena.get_base_ptr(source)->selection, state.now().selection), source, std::move(argument_list), is_mutable, is_pointer));
}

static core::ast::t_node_id parse_expr_parameter(parse_state& state) {
    const core::token& start_token = state.now();
   
    const core::ast::t_node_id name = state.arena.insert(core::ast::expr_identifier(state.expect(core::token_type::IDENTIFIER, "Expected an identifier.").selection));
    const core::ast::t_node_id type = parse_optional_type(state);
   
    core::ast::t_node_id default_value;

    if (state.now().type == core::token_type::EQUAL) {
        state.pos++;
        default_value = parse_expression(state);
    }
    else
        default_value = state.arena.insert(core::ast::expr_none(state.now().selection));

    return state.arena.insert(core::ast::expr_parameter(core::lisel(start_token.selection, state.now().selection), name, default_value, type));
}

static core::ast::t_node_id parse_expr_identifier(parse_state& state) {
    return state.arena.insert(core::ast::expr_identifier(state.expect(core::token_type::IDENTIFIER, "Expected an identifier.").selection));
}

static core::ast::t_node_id parse_expr_function(parse_state& state) {
    const core::token& start_token = state.now();

    core::ast::t_node_list type_parameter_list = parse_list<true, true>(state, parse_expr_identifier, L_TYPE_DELIMITER, R_TYPE_DELIMITER);
    core::ast::t_node_list parameter_list = parse_list<false, true>(state, parse_expr_parameter, L_FUNC_DELIMITER, R_FUNC_DELIMITER);
    const core::ast::t_node_id return_type = parse_optional_type(state);

    const core::ast::t_node_id body = parse_statement(state);

    return state.arena.insert(core::ast::expr_function(core::lisel(start_token.selection, state.now().selection), std::move(type_parameter_list), std::move(parameter_list), body, return_type));
}

static core::ast::t_node_id parse_primary_expression(parse_state& state) {
    switch (state.now().type) {
        case core::token_type::IDENTIFIER:
            return state.arena.insert(core::ast::expr_identifier(state.next().selection));
        case core::token_type::NUMBER:
            return state.arena.insert(core::ast::expr_literal(state.next().selection, core::ast::expr_literal::literal_type::NUMBER));
        case core::token_type::STRING:
            return state.arena.insert(core::ast::expr_literal(state.next().selection, core::ast::expr_literal::literal_type::STRING));
        case core::token_type::CHAR:
            return state.arena.insert(core::ast::expr_literal(state.next().selection, core::ast::expr_literal::literal_type::CHAR));
        case core::token_type::FALSE: [[fallthrough]];
        case core::token_type::TRUE:
            return state.arena.insert(core::ast::expr_literal(state.next().selection, core::ast::expr_literal::literal_type::BOOL));
        case core::token_type::NIL:
            return state.arena.insert(core::ast::expr_literal(state.next().selection, core::ast::expr_literal::literal_type::NIL));
        case core::token_type::DEC:
            return parse_variant_declaration(state, true);
        case L_EXPR_DELIMITER: {
            state.pos++;
            core::ast::t_node_id expr = parse_expression(state);
            state.expect(R_EXPR_DELIMITER, "Expected closing delimiter after expression.");
            return expr;
        }
        default:
            break;
    }

    state.log_and_pause_errors(core::lilog::log_level::ERROR, state.now().selection, "Unexpected token.");

    return state.arena.insert(core::ast::expr_invalid(state.next().selection));
}

static core::ast::t_node_id parse_scope_resolution(parse_state& state) {
    return binary_expression_left_associative(state, &parse_primary_expression, binary_scope_resolution_set);
}

static core::ast::t_node_id parse_member_access(parse_state& state) {
    return binary_expression_left_associative(state, &parse_scope_resolution, binary_member_access_set);
}

static core::ast::t_node_id parse_expr_call(parse_state& state) {
    const core::ast::t_node_id expression = parse_member_access(state);
    const core::ast::node_type expr_type = state.arena.get_base_ptr(expression)->type;

    if (
        expr_type != core::ast::node_type::EXPR_BINARY
        && expr_type != core::ast::node_type::EXPR_IDENTIFIER
        ||
        state.now().type != L_FUNC_DELIMITER
        && state.now().type != L_TYPE_DELIMITER
    )
        return expression;

    core::ast::t_node_list type_argument_list = parse_list<true, true>(state, parse_expr_type, L_TYPE_DELIMITER, R_TYPE_DELIMITER);
    core::ast::t_node_list argument_list = parse_list<false, true>(state, parse_expression, L_FUNC_DELIMITER, R_FUNC_DELIMITER);

    return state.arena.insert(core::ast::expr_call(core::lisel(state.arena.get_base_ptr(expression)->selection, state.now().selection), expression, std::move(type_argument_list), std::move(argument_list)));
}

static core::ast::t_node_id parse_expr_unary(parse_state& state) {
    const core::token& start_token = state.now();

    if (unary_pre_set.find(state.now().type) != unary_pre_set.end()) {
        const core::token& opr = state.next();
        core::ast::t_node_id operand = parse_expr_unary(state);
        return state.arena.insert(core::ast::expr_unary(core::lisel(start_token.selection, state.arena.get_base_ptr(operand)->selection), operand, opr, false));
    }

    const core::ast::t_node_id expression = parse_expr_call(state);

    if (unary_post_set.find(state.now().type) != unary_post_set.end()) {
        const core::token& opr = state.next();
        return state.arena.insert(core::ast::expr_unary(core::lisel(start_token.selection, opr.selection), expression, opr, true));
    }
   
    return expression;
}

static core::ast::t_node_id parse_exponential(parse_state& state) {
    return binary_expression_right_associative(state, &parse_expr_unary, binary_exponential_set);
}

static core::ast::t_node_id parse_multiplicative(parse_state& state) {
    return binary_expression_left_associative(state, &parse_exponential, binary_multiplicative_set);
}

static core::ast::t_node_id parse_additive(parse_state& state) {
    return binary_expression_left_associative(state, &parse_multiplicative, binary_additive_set);
}

static core::ast::t_node_id parse_numeric_comparison(parse_state& state) {
    return binary_expression_left_associative(state, &parse_additive, binary_numeric_comparison_set);
}

static core::ast::t_node_id parse_direct_comparison(parse_state& state) {
    return binary_expression_left_associative(state, &parse_numeric_comparison, binary_direct_comparison_set);
}

static core::ast::t_node_id parse_and(parse_state& state) {
    return binary_expression_left_associative(state, &parse_direct_comparison, binary_and_set);
}

static core::ast::t_node_id parse_or(parse_state& state) {
    return binary_expression_left_associative(state, &parse_and, binary_or_set);
}

static core::ast::t_node_id parse_expr_ternary(parse_state& state) {
    const core::ast::t_node_id first = parse_or(state);
    if (state.now().type != core::token_type::QUESTION)
        return first;
   
    state.pos++;
    const core::ast::t_node_id second = parse_expression(state);
    state.expect(core::token_type::COLON, "Expected a colon.");
    const core::ast::t_node_id third = parse_expression(state);
   
    return state.arena.insert(core::ast::expr_ternary(core::lisel(state.arena.get_base_ptr(first)->selection, state.arena.get_base_ptr(third)->selection), first, second, third));
}

static core::ast::t_node_id parse_assignment(parse_state& state) {
    return binary_expression_left_associative(state, &parse_expr_ternary, binary_assignment_set);
}

// Entry point to pratt parser design
static core::ast::t_node_id parse_expression(parse_state& state) {
    return parse_assignment(state);
}

static core::ast::t_node_id parse_stmt_if(parse_state& state) {
    const core::token& start_token = state.next();
    const core::ast::t_node_id condition = parse_expression(state);
    const core::ast::t_node_id consequent = parse_statement(state);
    core::ast::t_node_id alternate;

    if (state.now().type == core::token_type::ELSE) {
        state.pos++; // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = state.arena.insert(core::ast::stmt_none(state.now().selection));

    return state.arena.insert(core::ast::stmt_if(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate));
}

static core::ast::t_node_id parse_stmt_while(parse_state& state) {
    const core::token& start_token = state.next();
    const core::ast::t_node_id condition = parse_expression(state);
    const core::ast::t_node_id consequent = parse_statement(state);
    core::ast::t_node_id alternate;

    // In while loops, else's run if the condition fails on the first time.
    if (state.now().type == core::token_type::ELSE) {
        state.pos++; // Skip else
        alternate = parse_statement(state);
    }
    else
        alternate = state.arena.insert(core::ast::stmt_none(state.now().selection));

    return state.arena.insert(core::ast::stmt_while(core::lisel(start_token.selection, state.now().selection), condition, consequent, alternate));
}

template <typename T_NODE, typename PARSE_FUNC>
static core::ast::t_node_id parse_item_body(parse_state& state, PARSE_FUNC& parse_func) {
    const core::token& brace_token = state.now();
    core::ast::t_node_list item_list = parse_list<false, false>(state, parse_func, L_BODY_DELIMITER, R_BODY_DELIMITER);
       
    return state.arena.insert(T_NODE(core::lisel(brace_token.selection, state.now().selection), std::move(item_list)));
}

static core::ast::t_node_id parse_stmt_return(parse_state& state) {
    const core::token& start_token = state.next();

    core::ast::t_node_id expression;

    if (state.now().type == core::token_type::RBRACE)
        expression = state.arena.insert(core::ast::expr_none(state.now().selection));
    else
        expression = parse_expression(state);
   
    return state.arena.insert(core::ast::stmt_return(core::lisel(start_token.selection, state.arena.get_base_ptr(expression)->selection), expression));
}

static core::ast::t_node_id parse_item_use(parse_state& state) {
    const core::token& start_token = state.next();
    const core::token& value_token = state.expect(core::token_type::STRING, "Expected a string.");

    const core::ast::t_node_id value_node = state.arena.insert(core::ast::expr_literal(value_token.selection, core::ast::expr_literal::literal_type::STRING));

    return state.arena.insert(core::ast::item_use(core::lisel(start_token.selection, state.arena.get_base_ptr(value_node)->selection), value_node));
}

static core::ast::t_node_id parse_item_module(parse_state& state) {
    const core::token& start_token = state.next();
    const core::token& value_token = state.expect(core::token_type::IDENTIFIER, "Expected an identifier.");

    const core::ast::t_node_id name_node = state.arena.insert(core::ast::expr_identifier(value_token.selection));
    const core::ast::t_node_id content = parse_item(state);
   
    return state.arena.insert(core::ast::item_module(core::lisel(start_token.selection, state.arena.get_base_ptr(content)->selection), name_node, content));
}

static core::ast::t_node_id parse_variant_declaration(parse_state& state, const bool local_declaration) {
    const core::token& start_token = state.next();
   
    const core::ast::t_node_id name = parse_scope_resolution(state);
    const core::ast::t_node_id type = parse_optional_type(state);

    core::ast::t_node_id value;

    switch (state.now().type) {
        case L_TYPE_DELIMITER: // for potential type parameters
        case L_FUNC_DELIMITER:
            if (!local_declaration) {
                value = parse_expr_function(state);
                break;
            }
            value = state.arena.insert(core::ast::expr_invalid(state.now().selection));
            state.log_and_pause_errors(core::lilog::log_level::ERROR, state.next().selection, "Functions can not be declared in function bodies. Declare a closure instead.");
            value = state.arena.insert(core::ast::expr_invalid(state.now().selection));
            break;
        case core::token_type::EQUAL:
            state.pos++;
            value = parse_expression(state);
            break;
        default:
            value = state.arena.insert(core::ast::expr_none(state.now().selection));
    }

    return state.arena.insert(core::ast::variant_declaration(core::lisel(start_token.selection, state.now().selection), name, value, type));
}

static core::ast::t_node_id parse_item_type_declaration(parse_state& state) {
    const core::token& start_token = state.next();
   
    const core::ast::t_node_id name = parse_scope_resolution(state);
    core::ast::t_node_list parameter_list = parse_list<true, true>(state, parse_expr_identifier, L_TYPE_DELIMITER, R_TYPE_DELIMITER);

    state.expect(core::token_type::EQUAL, "Expected '='.");

    const core::ast::t_node_id value = parse_expr_type(state);

    return state.arena.insert(core::ast::item_type_declaration(core::lisel(start_token.selection, state.now().selection), name, value, std::move(parameter_list)));
}

// Find statements expected in a module or a struct.
static core::ast::t_node_id parse_item(parse_state& state) {
    state.f_pause_errors = false;

    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::USE: return parse_item_use(state);
        case core::token_type::MODULE: return parse_item_module(state);
        case core::token_type::DEC: return parse_variant_declaration(state, false);
        case core::token_type::TYPEDEC: return parse_item_type_declaration(state);
        case L_BODY_DELIMITER: return parse_item_body<core::ast::item_body>(state, parse_item);
        default: {
            const core::ast::node* statement = state.arena.get_base_ptr(parse_statement(state));
            state.log_and_pause_errors(core::lilog::log_level::ERROR, statement->selection, "The given item can only be used in a function body.");
            return state.arena.insert(core::ast::item_invalid(statement->selection));
        }
    }
}

// Find statements expected in function bodies.
static core::ast::t_node_id parse_statement(parse_state& state) {
    state.f_pause_errors = false;

    const core::token& tok = state.now();

    switch (tok.type) {
        case core::token_type::IF: return parse_stmt_if(state);
        case core::token_type::WHILE: return parse_stmt_while(state);
        case L_BODY_DELIMITER: return parse_item_body<core::ast::item_body>(state, parse_statement);
        case core::token_type::RETURN: return parse_stmt_return(state);
        case core::token_type::TYPEDEC: return parse_item_type_declaration(state);
        case core::token_type::BREAK: return state.arena.insert(core::ast::stmt_break(state.next().selection));
        case core::token_type::CONTINUE: return state.arena.insert(core::ast::stmt_continue(state.next().selection));

        // Capture this for items that are not statement compatible
        case core::token_type::USE:
        case core::token_type::MODULE:
            state.log_and_pause_errors(core::lilog::log_level::ERROR, tok.selection, "The given item can not be used in a function body.");
            return state.arena.insert(core::ast::stmt_invalid(state.next().selection));

        // Reserve the default case for expression wrapping.
        default: {
            core::ast::t_node_id expr_id = parse_expression(state);
           
            if (!state.arena.is_expression_wrappable(expr_id)) {
                const core::ast::node* as_node = state.arena.get_base_ptr(expr_id);
                state.log_and_pause_errors(core::lilog::log_level::ERROR, as_node->selection, "Unexpected expression.");
                return state.arena.insert(core::ast::stmt_invalid(as_node->selection));
            }
           
            return expr_id;
        }
    }
}

bool core::frontend::parse(core::liprocess& process, const core::t_file_id file_id) {
    parse_state state(process, file_id);
   
    state.arena.insert(core::ast::ast_root());
   
    while (!state.at_eof()) {
        auto result = parse_item(state);

        state.arena.get_as<core::ast::ast_root>(0).item_list.push_back(std::move(result));
    }

    state.file.dump_ast_arena = std::any(std::move(state.arena));

    return true;
}