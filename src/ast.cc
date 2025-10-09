#include "ast.hh"

bool core::ast::ast_arena::is_expression_wrappable(const t_node_id id) {
    const node* base = get_base_ptr(id);

    switch (base->type) {
        case node_type::EXPR_UNARY: {
            const token_type opr_type = ((expr_unary*)base)->opr.type;
            return opr_type == token_type::DOUBLE_PLUS || opr_type == token_type::DOUBLE_MINUS;
        }
        case node_type::EXPR_BINARY: {
            const token_type opr_type = ((expr_binary*)base)->opr.type;

            return opr_type == token_type::EQUAL;
        }
        case node_type::EXPR_CALL:
        case node_type::VARIANT_DECLARATION:
            return true;
    }

    return false;
}

/* 

Transparency note:
The debug function below was written with copilot or another AI source due to the almost absolute uselessness of writing it by hand.
Everything else is all monoglint B)

*/

void core::ast::ast_arena::pretty_debug(const liprocess& process, const t_node_id id, std::string& buffer, uint8_t indent) {
    const arena_node& an = node_list[id];
    const node* base = get_base_ptr(id);

    if (!base) {
        buffer += liutil::indent_repeat(indent) + "<null node>\n";
        return;
    }

    switch (base->type) {
        case node_type::ROOT: {
            const auto& v = std::get<ast_root>(an._raw);
            buffer += liutil::indent_repeat(indent) + "lican/ast_root : node\n";
            buffer += liutil::indent_repeat(indent+1) + "items:\n";
            for (const t_node_id& sid : v.item_list) pretty_debug(process, sid, buffer, indent+2);
            break;
        }

        case node_type::EXPR_NONE:
            buffer += liutil::indent_repeat(indent) + "expr_none\n";
            break;

        case node_type::EXPR_INVALID:
            buffer += liutil::indent_repeat(indent) + "expr_invalid\n";
            break;

        case node_type::EXPR_TYPE: {
            const auto& v = std::get<expr_type>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_type\n";
            buffer += liutil::indent_repeat(indent+1) + "source:\n";
            pretty_debug(process, v.source, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "is_const: " + (v.is_const ? "true" : "false") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "is_pointer: " + (v.is_pointer ? "true" : "false") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "reference_type: ";
            switch (v.reference_type) {
                case expr_type::e_reference_type::NONE: buffer += "NONE\n"; break;
                case expr_type::e_reference_type::LVALUE: buffer += "LVALUE\n"; break;
                case expr_type::e_reference_type::RVALUE: buffer += "RVALUE\n"; break;
                default: buffer += "UNKNOWN\n"; break;
            }
            buffer += liutil::indent_repeat(indent+1) + "arguments:\n";
            for (const t_node_id& a : v.argument_list) pretty_debug(process, a, buffer, indent+2);
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
            buffer += liutil::indent_repeat(indent+1) + "literal_type: ";
            switch (v.literal_type) {
                case expr_literal::e_literal_type::FLOAT: buffer += "FLOAT\n"; break;
                case expr_literal::e_literal_type::INT: buffer += "INT\n"; break;
                case expr_literal::e_literal_type::STRING: buffer += "STRING\n"; break;
                case expr_literal::e_literal_type::CHAR: buffer += "CHAR\n"; break;
                case expr_literal::e_literal_type::BOOL: buffer += "BOOL\n"; break;
                case expr_literal::e_literal_type::NIL: buffer += "NIL\n"; break;
                default: buffer += "UNKNOWN\n"; break;
            }
            break;
        }

        case node_type::EXPR_UNARY: {
            const auto& v = std::get<expr_unary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_unary\n";
            buffer += liutil::indent_repeat(indent+1) + "opr: " + process.sub_source_code(v.opr.selection) + (v.post ? " (post)" : " (pre)") + '\n';
            buffer += liutil::indent_repeat(indent+1) + "operand:\n";
            pretty_debug(process, v.operand, buffer, indent+2);
            break;
        }

        case node_type::EXPR_BINARY: {
            const auto& v = std::get<expr_binary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_binary\n";
            buffer += liutil::indent_repeat(indent+1) + "opr: " + process.sub_source_code(v.opr.selection) + '\n';
            buffer += liutil::indent_repeat(indent+1) + "first:\n";
            pretty_debug(process, v.first, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "second:\n";
            pretty_debug(process, v.second, buffer, indent+2);
            break;
        }

        case node_type::EXPR_TERNARY: {
            const auto& v = std::get<expr_ternary>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_ternary\n";
            buffer += liutil::indent_repeat(indent+1) + "first:\n";
            pretty_debug(process, v.first, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "second:\n";
            pretty_debug(process, v.second, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "third:\n";
            pretty_debug(process, v.third, buffer, indent+2);
            break;
        }

        case node_type::EXPR_PARAMETER: {
            const auto& v = std::get<expr_parameter>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_parameter\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "default_value:\n";
            pretty_debug(process, v.default_value, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            pretty_debug(process, v.value_type, buffer, indent+2);
            break;
        }

        case node_type::EXPR_FUNCTION: {
            const auto& v = std::get<expr_function>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_function\n";
            buffer += liutil::indent_repeat(indent+1) + "template_parameter_list:\n";
            for (const t_node_id& p : v.template_parameter_list) pretty_debug(process, p, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "parameter_list:\n";
            for (const t_node_id& p : v.parameter_list) pretty_debug(process, p, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "return_type:\n";
            pretty_debug(process, v.return_type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "body:\n";
            pretty_debug(process, v.body, buffer, indent+2);
            break;
        }

        case node_type::EXPR_CLOSURE: {
            buffer += liutil::indent_repeat(indent) + "expr_closure (not implemented)\n";
            break;
        }

        case node_type::EXPR_CALL: {
            const auto& v = std::get<expr_call>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_call\n";
            buffer += liutil::indent_repeat(indent+1) + "callee:\n";
            pretty_debug(process, v.callee, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "template_argument_list:\n";
            for (const t_node_id& a : v.template_argument_list) pretty_debug(process, a, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "argument_list:\n";
            for (const t_node_id& a : v.argument_list) pretty_debug(process, a, buffer, indent+2);
            break;
        }

        case node_type::STMT_NONE:
            buffer += liutil::indent_repeat(indent) + "stmt_none\n";
            break;

        case node_type::STMT_INVALID:
            buffer += liutil::indent_repeat(indent) + "stmt_invalid\n";
            break;

        case node_type::STMT_IF: {
            const auto& v = std::get<stmt_if>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_if\n";
            buffer += liutil::indent_repeat(indent+1) + "condition:\n";
            pretty_debug(process, v.condition, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "consequent:\n";
            pretty_debug(process, v.consequent, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "alternate:\n";
            pretty_debug(process, v.alternate, buffer, indent+2);
            break;
        }

        case node_type::STMT_WHILE: {
            const auto& v = std::get<stmt_while>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_while\n";
            buffer += liutil::indent_repeat(indent+1) + "condition:\n";
            pretty_debug(process, v.condition, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "consequent:\n";
            pretty_debug(process, v.consequent, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "alternate:\n";
            pretty_debug(process, v.alternate, buffer, indent+2);
            break;
        }

        case node_type::STMT_RETURN: {
            const auto& v = std::get<stmt_return>(an._raw);
            buffer += liutil::indent_repeat(indent) + "stmt_return\n";
            buffer += liutil::indent_repeat(indent+1) + "expression:\n";
            pretty_debug(process, v.expression, buffer, indent+2);
            break;
        }

        case node_type::ITEM_BODY: {
            const auto& v = std::get<item_body>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_body\n";
            buffer += liutil::indent_repeat(indent+1) + "items:\n";
            for (const t_node_id& s : v.item_list) pretty_debug(process, s, buffer, indent+2);
            break;
        }

        case node_type::STMT_BREAK:
            buffer += liutil::indent_repeat(indent) + "stmt_break\n";
            break;

        case node_type::STMT_CONTINUE:
            buffer += liutil::indent_repeat(indent) + "stmt_continue\n";
            break;

        case node_type::ITEM_USE: {
            const auto& v = std::get<item_use>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_use\n";
            buffer += liutil::indent_repeat(indent+1) + "path:\n";
            pretty_debug(process, v.path, buffer, indent+2);
            break;
        }

        case node_type::ITEM_MODULE: {
            const auto& v = std::get<item_module>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_module\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "content:\n";
            pretty_debug(process, v.content, buffer, indent+2);
            break;
        }

        case node_type::VARIANT_DECLARATION: {
            const auto& v = std::get<variant_declaration>(an._raw);
            buffer += liutil::indent_repeat(indent) + "variant_declaration\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            pretty_debug(process, v.value_type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value:\n";
            pretty_debug(process, v.value, buffer, indent+2);
            break;
        }

        case node_type::ITEM_TYPE_DECLARATION: {
            const auto& v = std::get<item_type_declaration>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_type_declaration\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "parameters:\n";
            for (const t_node_id& a : v.parameter_list) pretty_debug(process, a, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "type:\n";
            pretty_debug(process, v.type_value, buffer, indent+2);
            break;
        }

        case node_type::EXPR_PROPERTY: {
            const auto& v = std::get<expr_property>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_property\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value_type:\n";
            pretty_debug(process, v.value_type, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "default_value:\n";
            pretty_debug(process, v.default_value, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "is_private: " + (v.is_private ? "true" : "false") + '\n';
            break;
        }

        case node_type::EXPR_METHOD: {
            const auto& v = std::get<expr_method>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_method\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "function:\n";
            pretty_debug(process, v.function, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "is_private: " + (v.is_private ? "true" : "false") + '\n';
            break;
        }

        case node_type::EXPR_OPERATOR: {
            const auto& v = std::get<expr_operator>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_operator\n";
            buffer += liutil::indent_repeat(indent+1) + "opr: " + std::to_string(static_cast<int>(v.opr)) + '\n';
            buffer += liutil::indent_repeat(indent+1) + "function:\n";
            pretty_debug(process, v.function, buffer, indent+2);
            break;
        }

        case node_type::EXPR_INITIALIZER_SET: {
            const auto& v = std::get<expr_initializer_set>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_initializer_set\n";
            buffer += liutil::indent_repeat(indent+1) + "property_name:\n";
            pretty_debug(process, v.property_name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value:\n";
            pretty_debug(process, v.value, buffer, indent+2);
            break;
        }

        case node_type::EXPR_CONSTRUCTOR: {
            const auto& v = std::get<expr_constructor>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_constructor\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "function:\n";
            pretty_debug(process, v.function, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "initializer_list:\n";
            for (const t_node_id& it : v.initializer_list) pretty_debug(process, it, buffer, indent+2);
            break;
        }

        case node_type::EXPR_DESTRUCTOR: {
            const auto& v = std::get<expr_destructor>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_destructor\n";
            buffer += liutil::indent_repeat(indent+1) + "body:\n";
            pretty_debug(process, v.body, buffer, indent+2);
            break;
        }

        case node_type::ITEM_STRUCT_DECLARATION: {
            const auto& v = std::get<item_struct_declaration>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_struct_declaration\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "template_parameters:\n";
            for (const t_node_id& p : v.template_parameter_list) pretty_debug(process, p, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "members:\n";
            for (const t_node_id& m : v.member_list) pretty_debug(process, m, buffer, indent+2);
            break;
        }

        case node_type::EXPR_ENUM_SET: {
            const auto& v = std::get<expr_enum_set>(an._raw);
            buffer += liutil::indent_repeat(indent) + "expr_enum_set\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "value:\n";
            pretty_debug(process, v.value, buffer, indent+2);
            break;
        }

        case node_type::ITEM_ENUM: {
            const auto& v = std::get<item_enum>(an._raw);
            buffer += liutil::indent_repeat(indent) + "item_enum\n";
            buffer += liutil::indent_repeat(indent+1) + "name:\n";
            pretty_debug(process, v.name, buffer, indent+2);
            buffer += liutil::indent_repeat(indent+1) + "set_list:\n";
            for (const t_node_id& s : v.set_list) pretty_debug(process, s, buffer, indent+2);
            break;
        }

        case node_type::ITEM_INVALID:
            buffer += liutil::indent_repeat(indent) + "item_invalid\n";
            break;

        default:
            buffer += liutil::indent_repeat(indent) + "<unknown node>\n";
            break;
    }
}