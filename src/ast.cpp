#include "ast.hpp"

void core::ast::ast_arena::pretty_debug(const liprocess& process, const node_id id, std::string& buffer, uint8_t indent) {
    const arena_node& an = node_list[id];

    const node* base = get_ptr(id);
    if (!base) {
        buffer += liutil::indent_repeat(indent) + "<null node>\n";
        return;
    }

    switch (base->type) {
        case node_type::ROOT: {
            const auto& v = std::get<ast_root>(an._raw);
            buffer += liutil::indent_repeat(indent) + "lican/ast_root : node\n";
            buffer += liutil::indent_repeat(indent+1) + "scoped statements:\n";
            for (const node_id& sid : v.statement_list) pretty_debug(process, sid, buffer, indent+2);
            break;
        }
        case node_type::EXPR_NONE: {
            buffer += liutil::indent_repeat(indent) + "NONE\n";
            break;
        }
        case node_type::EXPR_INVALID: {
            buffer += liutil::indent_repeat(indent) + "expr_invalid\n";
            break;
        }
        case node_type::EXPR_TYPE: {
            const auto& v = std::get<expr_type>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_type\n";
            buffer += liutil::indent_repeat(indent+1) + "source:\n";
            this->pretty_debug(process, v.source, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "is_mutable: " + (v.is_mutable ? "true" : "false") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "is_reference: " + (v.is_reference ? "true" : "false") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "arguments:\n";
            for (const node_id& a : v.argument_list) this->pretty_debug(process, a, buffer, indent+2);
            break;
        }
        case node_type::EXPR_IDENTIFIER: {
            const auto& v = std::get<expr_identifier>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_identifier (" + process.sub_source_code(v.selection) + ")\n";
            break;
        }
        case node_type::EXPR_LITERAL: {
            const auto& v = std::get<expr_literal>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_literal (" + process.sub_source_code(v.selection) + ")\n";
            break;
        }
        case node_type::EXPR_UNARY: {
            const auto& v = std::get<expr_unary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_unary\n";
            buffer += liutil::indent_repeat(indent+1) + "opr: " + process.sub_source_code(v.opr.selection) + (v.post ? " (post)" : " (pre)") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "operand:\n";
            this->pretty_debug(process, v.operand, buffer, indent+2);
            break;
        }
        case node_type::EXPR_BINARY: {
            const auto& v = std::get<expr_binary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_binary\n";
            buffer += liutil::indent_repeat(indent+1) + "opr: " + process.sub_source_code(v.opr.selection) + '\n';
            buffer += liutil::indent_repeat(indent+1) + "first:\n";
            this->pretty_debug(process, v.first, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "second:\n";
            this->pretty_debug(process, v.second, buffer, indent+2);
            break;
        }
        case node_type::EXPR_TERNARY: {
            const auto& v = std::get<expr_ternary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_ternary\n";
            buffer += liutil::indent_repeat(indent+1) + "first:\n";
            this->pretty_debug(process, v.first, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "second:\n";
            this->pretty_debug(process, v.second, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "third:\n";
            this->pretty_debug(process, v.third, buffer, indent+2);
            break;
        }
        case node_type::EXPR_PARAMETER: {
            const auto& v = std::get<expr_parameter>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_parameter\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            this->pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "default_value:\n";
            this->pretty_debug(process, v.default_value, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            this->pretty_debug(process, v.type, buffer, indent+2);
            break;
        }
        case node_type::EXPR_FUNCTION: {
            const auto& v = std::get<expr_function>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_function\n";
            buffer += liutil::indent_repeat(indent+1) + "parameter_list:\n";
            for (const node_id& p : v.parameter_list) this->pretty_debug(process, p, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "return_type:\n";
            this->pretty_debug(process, v.return_type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "body:\n";
            this->pretty_debug(process, v.body, buffer, indent+2);
            break;
        }
        case node_type::EXPR_CALL: {
            const auto& v = std::get<expr_call>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_call\n";
            buffer += liutil::indent_repeat(indent+1) + "callee:\n";
            this->pretty_debug(process, v.callee, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "argument_list:\n";
            for (const node_id& a : v.argument_list) this->pretty_debug(process, a, buffer, indent+2);
            break;
        }
        case node_type::EXPR_LOCAL_DECLARATION: {
            const auto& v = std::get<expr_local_declaration>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_local_declaration\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            this->pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            this->pretty_debug(process, v.type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value:\n";
            this->pretty_debug(process, v.value, buffer, indent+2);
            break;
        }
        case node_type::STMT_NONE: {
            buffer += liutil::indent_repeat(indent) + "NONE\n";
            break;
        }
        case node_type::STMT_INVALID: {
            buffer += liutil::indent_repeat(indent) + "stmt_invalid\n";
            break;
        }
        case node_type::STMT_IF: {
            const auto& v = std::get<stmt_if>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_if\n";
            buffer += liutil::indent_repeat(indent+1) + "condition:\n";
            this->pretty_debug(process, v.condition, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "consequent:\n";
            this->pretty_debug(process, v.consequent, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "alternate:\n";
            this->pretty_debug(process, v.alternate, buffer, indent+2);
            break;
        }
        case node_type::STMT_WHILE: {
            const auto& v = std::get<stmt_while>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_while\n";
            buffer += liutil::indent_repeat(indent+1) + "condition:\n";
            this->pretty_debug(process, v.condition, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "consequent:\n";
            this->pretty_debug(process, v.consequent, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "alternate:\n";
            this->pretty_debug(process, v.alternate, buffer, indent+2);
            break;
        }
        case node_type::STMT_RETURN: {
            const auto& v = std::get<stmt_return>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_return\n";
            buffer += liutil::indent_repeat(indent+1) + "expression:\n";
            this->pretty_debug(process, v.expression, buffer, indent+2);
            break;
        }
        case node_type::STMT_WRAPPER: {
            const auto& v = std::get<stmt_wrapper>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_wrapper\n";
            this->pretty_debug(process, v.expression, buffer, indent+1);
            break;
        }
        case node_type::STMT_BODY: {
            const auto& v = std::get<stmt_body>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_body\n";
            buffer += liutil::indent_repeat(indent+1) + "statements:\n";
            for (const node_id& s : v.statement_list) this->pretty_debug(process, s, buffer, indent+2);
            break;
        }
        case node_type::STMT_BREAK: {
            buffer += liutil::indent_repeat(indent) + "stmt_break\n";
            break;
        }
        case node_type::STMT_CONTINUE: {
            buffer += liutil::indent_repeat(indent) + "stmt_continue\n";
            break;
        }
        case node_type::S_STMT_USE: {
            const auto& v = std::get<s_stmt_use>(an._raw);
            buffer += liutil::indent_repeat(indent) + "s_stmt_use\n";
            buffer += liutil::indent_repeat(indent+1) + "path:\n";
            this->pretty_debug(process, v.path, buffer, indent+2);
            break;
        }
        case node_type::S_STMT_SCOPED_BODY: {
            const auto& v = std::get<s_stmt_scoped_body>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_scoped_body\n";
            buffer += liutil::indent_repeat(indent+1) + "scoped statements:\n";
            for (const node_id& s : v.statement_list) pretty_debug(process, s, buffer, indent+2);
            break;
        }
        case node_type::S_STMT_NAMESPACE: {
            const auto& v = std::get<s_stmt_namespace>(an._raw);
            buffer += liutil::indent_repeat(indent) + "s_stmt_namespace\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            this->pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "content:\n";
            this->pretty_debug(process, v.content, buffer, indent+2);
            break;
        }
        case node_type::S_STMT_DECLARATION: {
            const auto& v = std::get<s_stmt_declaration>(an._raw);
            buffer += liutil::indent_repeat(indent) + "s_stmt_declaration\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            this->pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            this->pretty_debug(process, v.type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value:\n";
            this->pretty_debug(process, v.value, buffer, indent+2);
            break;
        }
        case node_type::S_STMT_INVALID: {
            buffer += liutil::indent_repeat(indent) + "s_stmt_invalid\n";
            break;
        }
        default: {
            buffer += liutil::indent_repeat(indent) + "<unknown node>\n";
            break;
        }
    }
}

// void core::ast::ast_arena::pretty_debug(const liprocess& process, const node* n, std::string& buffer, uint8_t indent = 0) const {
//     if (n == nullptr) {
//         buffer += liutil::indent_repeat(indent) + "<null node>\n";
//         return;
//     }

//     // Find the matching stored node by pointer comparison.
//     for (node_id i = 0; i < node_list.size(); ++i) {
//         if (get_ptr(i) == n) {
//             pretty_debug(process, i, buffer, indent);
//             return;
//         }
//     }

//     // Not found in arena: fallback to printing basic info using type field.
//     buffer += liutil::indent_repeat(indent) + "<external node type=" + std::to_string(static_cast<int>(n->type)) + ">\n";
// }