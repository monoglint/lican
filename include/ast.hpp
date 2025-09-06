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

            EXPR_INVALID,
            EXPR_IDENTIFIER,
            EXPR_LITERAL,

            STMT_NONE,
            STMT_IF,
            STMT_WRAPPER,
            STMT_BODY,
        };

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

        struct expr_invalid : expr {
            expr_invalid(const core::lisel& selection)
                : expr(selection, node_type::EXPR_INVALID) {}
            
            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_invalid\n";
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
            };

            expr_literal(const core::lisel& selection, const literal_type type)
                : expr(selection, node_type::EXPR_IDENTIFIER), type(type) {}

            const literal_type type;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "expr_literal (" + process.sub_source_code(selection) + ")\n";
            }
        };

        struct expr_binary : expr {
            expr_binary(const core::lisel& selection, p_expr&& left, p_expr&& right, const core::token& opr)
                : expr(selection, node_type::EXPR_IDENTIFIER), left(std::move(left)), right(std::move(right)), opr(opr) {}

            const p_expr left;
            const p_expr right;
            const core::token opr;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "expr_binary\n";
                buffer += _indent(indent) + "opr: " + process.sub_source_code(opr.selection) + '\n';
                buffer += _indent(indent) + "left\n";
                left->pretty_debug(process, buffer, indent + 1);
                buffer += _indent(indent) + "right\n";
                right->pretty_debug(process, buffer, indent + 1);
            }
        };

        struct stmt_none : stmt {
            stmt_none(const core::lisel& selection)
                : stmt(selection, node_type::STMT_NONE) {}

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "NONE\n"; 
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

        struct stmt_wrapper : stmt {
            stmt_wrapper(p_expr&& expression)
                : stmt(expression->selection, node_type::STMT_WRAPPER), expression(std::move(expression)) {}

            const p_expr expression;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent++) + "stmt_wrapper\n";
                expression->pretty_debug(process, buffer, indent);
            }
        };

        struct stmt_body : stmt {
            stmt_body(const core::lisel& selection, std::vector<p_stmt>& statement_list)
                : stmt(selection, node_type::ROOT), statement_list(std::move(statement_list)) {}

            std::vector<p_stmt> statement_list;

            inline void pretty_debug(const liprocess& process, std::string& buffer, uint8_t indent = 0) const override {
                buffer += _indent(indent) + "stmt_body : node\n";
                buffer += _indent(indent++) + "statements:\n";

                for (const p_stmt& stmt : statement_list) {
                    stmt->pretty_debug(process, buffer, indent);
                }
            }
        };
    }
}

#undef IS_AST_HEADER