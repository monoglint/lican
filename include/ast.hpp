// Contains all of the AST node declarations. Used by the frontend and read by the backend.
#pragma once

#include <cstdint>
#include <memory>

#include "core.hpp"
#include "token.hpp"

#define IS_AST_HEADER

#ifdef IS_AST_HEADER
inline std::string _indent(const uint8_t level) {
	std::string buffer;

	for (uint8_t i = 0; i < level; i++) {
		buffer += ".  ";
	}

	return buffer;
}
#endif

namespace core {
    namespace ast {
        enum class node_type : uint8_t {
            ROOT,

            EXPR_NONE,
            EXPR_INVALID,
            EXPR_TYPE,
            EXPR_IDENTIFIER,
            EXPR_LITERAL,
            EXPR_UNARY,
            EXPR_BINARY,
            EXPR_TERNARY,
            
            EXPR_PARAMETER,
            EXPR_FUNCTION,
            EXPR_CLOSURE,
            
            EXPR_LOCAL_DECLARATION,

            EXPR_CALL,

            STMT_NONE,
            STMT_INVALID,
            STMT_IF,
            STMT_WHILE,
            STMT_RETURN,
            STMT_WRAPPER,
            STMT_BODY,
            STMT_BREAK,
            STMT_CONTINUE,

            S_STMT_USE,
            S_STMT_SCOPED_BODY,
            S_STMT_NAMESPACE,
            S_STMT_DECLARATION,
            S_STMT_INVALID,
        };
        
        // AST nodes

        struct node {
            node(const core::lisel& selection, const node_type type)
                : selection(selection), type(type) {}

            node_type type;
            core::lisel selection;

            inline virtual void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const {
                buffer += '\n';
            }
        };

        struct stmt : node {
            stmt(const core::lisel& selection, const node_type type)
                : node(selection, type) {}
        };

        struct s_stmt : node {
            s_stmt(const core::lisel& selection, const node_type type)
                : node(selection, type) {}
        };

        struct expr : node {
            expr(const core::lisel& selection, const node_type type)
                : node(selection, type) {}

            inline virtual bool is_wrappable() const {
                return false;
            }
        };

        using up_node = std::unique_ptr<node>;
        using up_stmt = std::unique_ptr<stmt>;
        using up_s_stmt = std::unique_ptr<s_stmt>;
        using up_expr = std::unique_ptr<expr>;

        struct ast_root : node {
            ast_root()
                : node(core::lisel(0, 0), node_type::ROOT) {}

            std::vector<up_s_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "lican/ast_root : node\n";
                buffer += _indent(indent++) + "scoped statements:\n";

                for (const up_s_stmt& stmt : statement_list) {
                    stmt->pretty_debug(process, buffer, indent);
                }
            }
        };

        struct expr_none : expr {
            expr_none(const core::lisel& selection)
                : expr(selection, node_type::EXPR_NONE) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "NONE\n"; 
            }
        };
        
        struct expr_invalid : expr {
            expr_invalid(const core::lisel& selection)
                : expr(selection, node_type::EXPR_INVALID) {}
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_invalid\n";
            }
        };

        struct expr_type : expr {
            expr_type(const core::lisel& selection, up_expr&& source, std::vector<std::unique_ptr<expr_type>>&& argument_list, const bool is_mutable, const bool is_reference)
                : expr(selection, node_type::EXPR_TYPE), source(std::move(source)), argument_list(std::move(argument_list)), is_mutable(is_mutable), is_reference(is_reference) {}
            
            up_expr source; // Identifier or scope resolution
            std::vector<std::unique_ptr<expr_type>> argument_list;
            
            bool is_mutable;
            bool is_reference;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_type\n";
                buffer += _indent(indent) + "source:\n";
                source->pretty_debug(process, buffer, indent + 1);

                buffer += _indent(indent) + "is_mutable: " + (is_mutable ? "true" : "false") + '\n';
                buffer += _indent(indent) + "is_reference: " + (is_reference ? "true" : "false") + '\n';

                buffer += _indent(indent) + "arguments:\n";
                for (const auto& arg : argument_list) {
                    arg->pretty_debug(process, buffer, indent + 1);
                }
            }
        };

        struct expr_identifier : expr {
            expr_identifier(const core::lisel& selection)
                : expr(selection, node_type::EXPR_IDENTIFIER) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_identifier (" + process.sub_source_code(selection) + ")\n";
            }
        };

        struct expr_literal : expr {
            enum class literal_type : uint8_t {
                NUMBER,
                STRING,
                CHAR,
                BOOL,
                NIL,
            };

            expr_literal(const core::lisel& selection, const literal_type type)
                : expr(selection, node_type::EXPR_LITERAL), type(type) {}

            literal_type type;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_literal (" + process.sub_source_code(selection) + ")\n";
            }
        };

        struct expr_unary : expr {
            expr_unary(const core::lisel& selection, up_expr&& operand, const core::token& opr, const bool post)
                : expr(selection, node_type::EXPR_UNARY), operand(std::move(operand)), opr(opr), post(post) {}

            up_expr operand;
            core::token opr;
            bool post;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_unary\n";
                buffer += _indent(indent) + "opr: " + process.sub_source_code(opr.selection) + (post ? " (post)" : " (pre)") + '\n';
                buffer += _indent(indent) + "operand\n";
                operand->pretty_debug(process, buffer, indent + 1);
            }

            inline bool is_wrappable() const override {
                return opr.type == token_type::DOUBLE_PLUS || opr.type == token_type::DOUBLE_MINUS;
            }
        };

        struct expr_binary : expr {
            expr_binary(const core::lisel& selection, up_expr&& first, up_expr&& second, const core::token& opr)
                : expr(selection, node_type::EXPR_BINARY), first(std::move(first)), second(std::move(second)), opr(opr) {}

            up_expr first;
            up_expr second;
            core::token opr;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_binary\n";
                buffer += _indent(indent) + "opr: " + process.sub_source_code(opr.selection) + '\n';
                buffer += _indent(indent) + "first\n";
                first->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "second\n";
                second->pretty_debug(process, buffer, indent + 1);
            }
        };
        
        struct expr_ternary : expr {
            expr_ternary(const core::lisel& selection, up_expr&& first, up_expr&& second, up_expr&& third)
                : expr(selection, node_type::EXPR_TERNARY), first(std::move(first)), second(std::move(second)), third(std::move(third)) {}

            up_expr first;
            up_expr second;
            up_expr third;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_ternary\n";
                buffer += _indent(indent) + "first\n";
                first->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "second\n";
                second->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "third\n";
                third->pretty_debug(process, buffer, indent + 1);
            }
        };
        
        struct expr_parameter : expr {
            expr_parameter(const core::lisel& selection, std::unique_ptr<expr_identifier>&& name, up_expr&& default_value, up_expr&& type)
                : expr(selection, node_type::EXPR_PARAMETER), name(std::move(name)), default_value(std::move(default_value)), type(std::move(type)) {}

            std::unique_ptr<expr_identifier> name;
            up_expr default_value;
            up_expr type;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_parameter\n";
                buffer += _indent(indent) + "name:\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "default_value:\n";
                default_value->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "type:\n";
                type->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct expr_function : expr {
            expr_function(const core::lisel& selection, std::vector<std::unique_ptr<expr_parameter>>&& parameter_list, up_stmt&& body, up_expr&& return_type)
                : expr(selection, node_type::EXPR_FUNCTION), parameter_list(std::move(parameter_list)), body(std::move(body)), return_type(std::move(return_type)) {}

            std::vector<std::unique_ptr<expr_parameter>> parameter_list;
            up_stmt body;
            up_expr return_type;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_function\n";
                buffer += _indent(indent) + "parameter_list:\n";
                
                for (auto& param : parameter_list) {
                    param->pretty_debug(process, buffer, indent + 1);
                }

                buffer += _indent(indent) + "return_type:\n";
                return_type->pretty_debug(process, buffer, indent + 1);

                buffer += _indent(indent) + "body:\n";
                body->pretty_debug(process, buffer, indent + 1);
            } 
        };

        struct expr_call : expr {
            expr_call(const core::lisel& selection, up_expr&& callee, std::vector<up_expr>&& argument_list)
                : expr(selection, node_type::EXPR_CALL), callee(std::move(callee)), argument_list(std::move(argument_list)) {}

            up_expr callee;
            std::vector<up_expr> argument_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_call\n";
                buffer += _indent(indent) + "callee:\n";
                
                callee->pretty_debug(process, buffer, indent + 1);

                buffer += _indent(indent) + "argument_list:\n";
                for (auto& param : argument_list) {
                    param->pretty_debug(process, buffer, indent + 1);
                }
            }
            
            inline bool is_wrappable() const override {
                return true;
            }
        };

        struct expr_local_declaration : expr {
            expr_local_declaration(const core::lisel& selection, up_expr&& name, up_expr&& value, up_expr&& type)
                : expr(selection, node_type::EXPR_LOCAL_DECLARATION), name(std::move(name)), value(std::move(value)), type(std::move(type)) {}
            
            up_expr name; // Use up_expr instead of expr_identifier since variables can be declared within scopes.
            up_expr value;
            up_expr type;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_local_declaration\n";
                buffer += _indent(indent) + "name:\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "type:\n";
                type->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "value:\n";
                value->pretty_debug(process, buffer, indent + 1);
            }

            inline bool is_wrappable() const override {
                return true;
            }
        };
        
        struct stmt_none : stmt {
            stmt_none(const core::lisel& selection)
                : stmt(selection, node_type::STMT_NONE) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "NONE\n"; 
            }
        };

        struct stmt_invalid : stmt {
            stmt_invalid(const core::lisel& selection)
                : stmt(selection, node_type::STMT_INVALID) {}
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_invalid\n";
            }
        };

        struct stmt_if : stmt {
            stmt_if(const core::lisel& selection, up_expr&& condition, up_stmt&& consequent, up_stmt&& alternate)
                : stmt(selection, node_type::STMT_IF), condition(std::move(condition)), consequent(std::move(consequent)), alternate(std::move(alternate)) {}

            up_expr condition;
            up_stmt consequent;
            up_stmt alternate;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_if\n";
                buffer += _indent(indent) + "condition:\n";
                condition->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "consequent:\n";
                consequent->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "alternate:\n";
                alternate->pretty_debug(process, buffer, indent + 1);
            }
        };
        
        struct stmt_while : stmt {
            stmt_while(const core::lisel& selection, up_expr&& condition, up_stmt&& consequent, up_stmt&& alternate)
                : stmt(selection, node_type::STMT_WHILE), condition(std::move(condition)), consequent(std::move(consequent)), alternate(std::move(alternate)) {}

            up_expr condition;
            up_stmt consequent;
            up_stmt alternate; // yes, we have else clauses in while loops

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_while\n";
                buffer += _indent(indent) + "condition:\n";
                condition->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "consequent:\n";
                consequent->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "alternate:\n";
                alternate->pretty_debug(process, buffer, indent + 1);
            }
        };
        
        struct stmt_return : stmt {
            stmt_return(const core::lisel& selection, up_expr&& expression)
                : stmt(selection, node_type::STMT_RETURN), expression(std::move(expression)) {}
            
            up_expr expression;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_return\n";
                buffer += _indent(indent) + "expression:\n";
                expression->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct stmt_wrapper : stmt {
            stmt_wrapper(up_expr&& expression)
                : stmt(expression->selection, node_type::STMT_WRAPPER), expression(std::move(expression)) {}

            up_expr expression;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_wrapper\n";
                expression->pretty_debug(process, buffer, indent);
            }
        };

        struct stmt_body : stmt {
            stmt_body(const core::lisel& selection, std::vector<up_stmt>&& statement_list)
                : stmt(selection, node_type::STMT_BODY), statement_list(std::move(statement_list)) {}

            std::vector<up_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_body\n";
                buffer += _indent(indent++) + "statements:\n";

                for (const up_stmt& stmt : statement_list) {
                    stmt->pretty_debug(process, buffer, indent);
                }
            }
        };

        struct stmt_break : stmt {
            stmt_break(const core::lisel& selection)
                : stmt(selection, node_type::STMT_BREAK) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_break\n";
            }
        };

        struct stmt_continue : stmt {
            stmt_continue(const core::lisel& selection)
                : stmt(selection, node_type::STMT_CONTINUE) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_continue\n";
            }
        };

        struct s_stmt_use : s_stmt {
            s_stmt_use(const core::lisel& selection, std::unique_ptr<expr_literal>&& path)
                : s_stmt(selection, node_type::S_STMT_USE), path(std::move(path)) {}

            // The parser must ensure that this literal is a string.
            std::unique_ptr<expr_literal> path;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "s_stmt_use\n";
                path->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct s_stmt_scoped_body : s_stmt {
            s_stmt_scoped_body(const core::lisel& selection, std::vector<up_s_stmt>&& statement_list)
                : s_stmt(selection, node_type::S_STMT_SCOPED_BODY), statement_list(std::move(statement_list)) {}

            std::vector<up_s_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_scoped_body\n";
                buffer += _indent(indent++) + "scoped statements:\n";

                for (const up_s_stmt& stmt : statement_list) {
                    stmt->pretty_debug(process, buffer, indent);
                }
            }
        };

        struct s_stmt_namespace : s_stmt {
            s_stmt_namespace(const core::lisel& selection, std::unique_ptr<expr_identifier>&& name, up_s_stmt&& content)
                : s_stmt(selection, node_type::S_STMT_NAMESPACE), name(std::move(name)), content(std::move(content)) {}

            std::unique_ptr<expr_identifier> name;
            up_s_stmt content;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "s_stmt_namespace\n";
                buffer += _indent(indent) + "name\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "content:\n";
                content->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct s_stmt_declaration : s_stmt {
            s_stmt_declaration(const core::lisel& selection, up_expr&& name, up_expr&& value, up_expr&& type)
                : s_stmt(selection, node_type::S_STMT_DECLARATION), name(std::move(name)), value(std::move(value)), type(std::move(type)) {}
            
            up_expr name; // Use up_expr instead of expr_identifier since variables can be declared within scopes.
            up_expr value;
            up_expr type;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "s_stmt_declaration\n";
                buffer += _indent(indent) + "name:\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "type:\n";
                type->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "value:\n";
                value->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct s_stmt_invalid : s_stmt {
            s_stmt_invalid(const core::lisel& selection)
                : s_stmt(selection, node_type::EXPR_INVALID) {}
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "s_stmt_invalid\n";
            }
        };
    }
}

#undef IS_AST_HEADER