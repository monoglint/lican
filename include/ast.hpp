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
        enum class variable_qualifier : uint8_t {
            CONST,
            STATIC,
        };
        
        enum class parameter_qualifier : uint8_t {
            CONST,
        };
        
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
            
            EXPR_CALL,

            STMT_NONE,
            STMT_INVALID,
            STMT_IF,
            STMT_WHILE,
            STMT_DECLARATION,
            STMT_RETURN,
            STMT_WRAPPER,
            STMT_BODY,
            STMT_USE,
            STMT_BREAK,
            STMT_CONTINUE,
        };
        
        // AST nodes

        struct node {
            node(const core::lisel& selection, const node_type type)
                : selection(selection), type(type) {}

            const node_type type;
            const core::lisel selection;

            inline virtual void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const {
                buffer += '\n';
            }
        };

        struct stmt : node {
            stmt(const core::lisel& selection, const node_type type)
                : node(selection, type) {}
        };

        struct expr : node {
            expr(const core::lisel& selection, const node_type type)
                : node(selection, type) {}

            inline virtual bool is_wrappable() const {
                return false;
            }
        };

        using p_stmt = std::unique_ptr<stmt>;
        using p_expr = std::unique_ptr<expr>;

        struct ast_root : node {
            ast_root()
                : node(core::lisel(0, 0), node_type::ROOT) {}

            std::vector<p_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "lican/ast_root : node\n";
                buffer += _indent(indent++) + "statements:\n";

                for (const p_stmt& stmt : statement_list) {
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
            expr_type(const core::lisel& selection, p_expr& source, std::vector<std::unique_ptr<expr_type>>& argument_list)
                : expr(selection, node_type::EXPR_TYPE), source(std::move(source)), argument_list(std::move(argument_list)) {}
            
            const p_expr source;
            const std::vector<std::unique_ptr<expr_type>> argument_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_type\n";
                buffer += _indent(indent) + "source:\n";
                source->pretty_debug(process, buffer, indent + 1);
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

            const literal_type type;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_literal (" + process.sub_source_code(selection) + ")\n";
            }
        };

        struct expr_unary : expr {
            expr_unary(const core::lisel& selection, p_expr& operand, const core::token& opr, const bool post)
                : expr(selection, node_type::EXPR_UNARY), operand(std::move(operand)), opr(opr), post(post) {}

            const p_expr operand;
            const core::token opr;
            const bool post;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_unary\n";
                buffer += _indent(indent) + "opr: " + process.sub_source_code(opr.selection) + (post ? " (post)" : " (pre)") + '\n';
                buffer += _indent(indent) + "operand\n";
                operand->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct expr_binary : expr {
            expr_binary(const core::lisel& selection, p_expr&& first, p_expr&& second, const core::token& opr)
                : expr(selection, node_type::EXPR_BINARY), first(std::move(first)), second(std::move(second)), opr(opr) {}

            const p_expr first;
            const p_expr second;
            const core::token opr;

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
            expr_ternary(const core::lisel& selection, p_expr& first, p_expr& second, p_expr& third)
                : expr(selection, node_type::EXPR_TERNARY), first(std::move(first)), second(std::move(second)), third(std::move(third)) {}

            const p_expr first;
            const p_expr second;
            const p_expr third;

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
            expr_parameter(const core::lisel& selection, std::unique_ptr<expr_identifier>& name, p_expr& default_value, std::vector<parameter_qualifier>& qualifiers, p_expr& type)
                : expr(selection, node_type::EXPR_PARAMETER), name(std::move(name)), default_value(std::move(default_value)), qualifiers(std::move(qualifiers)), type(std::move(type)) {}

            const std::unique_ptr<expr_identifier> name;
            const p_expr default_value;
            const std::vector<parameter_qualifier> qualifiers;
            const p_expr type;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_parameter\n";
                buffer += _indent(indent) + "name:\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "default_value:\n";
                default_value->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "qualifiers: [NOT ADDED]\n";
                buffer += _indent(indent) + "type:\n";
                type->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct expr_function : expr {
            expr_function(const core::lisel& selection, std::vector<std::unique_ptr<expr_parameter>>& parameter_list, p_stmt& body, p_expr& return_type)
                : expr(selection, node_type::EXPR_FUNCTION), parameter_list(std::move(parameter_list)), body(std::move(body)), return_type(std::move(return_type)) {}

            const std::vector<std::unique_ptr<expr_parameter>> parameter_list;
            const p_stmt body;
            const p_expr return_type;

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
            expr_call(const core::lisel& selection, p_expr& callee, std::vector<p_expr>& argument_list)
                : expr(selection, node_type::EXPR_CALL), callee(std::move(callee)), argument_list(std::move(argument_list)) {}

            const p_expr callee;
            const std::vector<p_expr> argument_list;

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
            stmt_if(const core::lisel& selection, p_expr& condition, p_stmt& consequent, p_stmt& alternate)
                : stmt(selection, node_type::STMT_IF), condition(std::move(condition)), consequent(std::move(consequent)), alternate(std::move(alternate)) {}

            const p_expr condition;
            const p_stmt consequent;
            const p_stmt alternate;

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
            stmt_while(const core::lisel& selection, p_expr& condition, p_stmt& consequent, p_stmt& alternate)
                : stmt(selection, node_type::STMT_WHILE), condition(std::move(condition)), consequent(std::move(consequent)), alternate(std::move(alternate)) {}

            const p_expr condition;
            const p_stmt consequent;
            const p_stmt alternate; // yes, we have else clauses in while loops

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
        
        struct stmt_declaration : stmt {
            stmt_declaration(const core::lisel& selection, p_expr& name, p_expr& value, std::vector<variable_qualifier>& qualifiers, p_expr& type)
                : stmt(selection, node_type::STMT_DECLARATION), name(std::move(name)), value(std::move(value)), qualifiers(std::move(qualifiers)), type(std::move(type)) {}
            
            const p_expr name; // Use p_expr instead of expr_identifier since variables can be declared within scopes.
            const p_expr value;
            const std::vector<variable_qualifier> qualifiers;
            const p_expr type;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_declaration\n";
                buffer += _indent(indent) + "name:\n";
                name->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "type:\n";
                type->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "value:\n";
                value->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "qualifiers: [NOT ADDED]\n";
            }
        };
        
        struct stmt_return : stmt {
            stmt_return(const core::lisel& selection, p_expr& expression)
                : stmt(selection, node_type::STMT_RETURN), expression(std::move(expression)) {}
            
            const p_expr expression;
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_return\n";
                buffer += _indent(indent) + "expression:\n";
                expression->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct stmt_wrapper : stmt {
            stmt_wrapper(p_expr& expression)
                : stmt(expression->selection, node_type::STMT_WRAPPER), expression(std::move(expression)) {}

            const p_expr expression;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_wrapper\n";
                expression->pretty_debug(process, buffer, indent);
            }
        };

        struct stmt_body : stmt {
            stmt_body(const core::lisel& selection, std::vector<p_stmt>& statement_list)
                : stmt(selection, node_type::STMT_BODY), statement_list(std::move(statement_list)) {}

            const std::vector<p_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_body\n";
                buffer += _indent(indent++) + "statements:\n";

                for (const p_stmt& stmt : statement_list) {
                    stmt->pretty_debug(process, buffer, indent);
                }
            }
        };

        struct stmt_use : stmt {
            stmt_use(const core::lisel& selection, std::unique_ptr<expr_literal>& path)
                : stmt(selection, node_type::STMT_USE), path(std::move(path)) {};

            // The parser must ensure that this literal is a string.
            const std::unique_ptr<expr_literal> path;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_use\n";
                path->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct stmt_break : stmt {
            stmt_break(const core::lisel& selection)
                : stmt(selection, node_type::STMT_BREAK) {};

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_break\n";
            }
        };

        struct stmt_continue : stmt {
            stmt_continue(const core::lisel& selection)
                : stmt(selection, node_type::STMT_CONTINUE) {};

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_continue\n";
            }
        };
    }
}

#undef IS_AST_HEADER