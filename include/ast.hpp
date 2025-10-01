// Contains all of the AST node declarations. Used by the frontend and read by the backend.
#pragma once

#include <cstdint>
#include <memory>
#include <variant>

#include "util.hpp"
#include "core.hpp"
#include "token.hpp"

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

        using node_id = size_t;
        struct ast_arena;
        
        struct node {
            node(const core::lisel& selection, const node_type type)
                : selection(selection), type(type) {}

            node_type type;
            core::lisel selection;

            inline virtual void pretty_debug(const liprocess& process, const ast_arena& arena, std::string& buffer, uint8_t indent = 0) const {
                buffer += '\n';
            }
        };

        struct ast_root : node {
            ast_root()
                : node(core::lisel(0, 0), node_type::ROOT) {}

            std::vector<node_id> statement_list;
        };

        struct expr_none : node {
            expr_none(const core::lisel& selection)
                : node(selection, node_type::EXPR_NONE) {}
        };
        
        struct expr_invalid : node {
            expr_invalid(const core::lisel& selection)
                : node(selection, node_type::EXPR_INVALID) {}
        };

        struct expr_type : node {
            expr_type(const core::lisel& selection, const node_id source, std::vector<node_id>&& argument_list, const bool is_mutable, const bool is_reference)
                : node(selection, node_type::EXPR_TYPE), source(source), argument_list(std::move(argument_list)), is_mutable(is_mutable), is_reference(is_reference) {}
            
            node_id source; // Identifier or scope resolution
            std::vector<node_id> argument_list;
            
            bool is_mutable;
            bool is_reference;
        };

        struct expr_identifier : node {
            expr_identifier(const core::lisel& selection)
                : node(selection, node_type::EXPR_IDENTIFIER) {}
        };

        struct expr_literal : node {
            enum class literal_type : uint8_t {
                NUMBER,
                STRING,
                CHAR,
                BOOL,
                NIL,
            };

            expr_literal(const core::lisel& selection, const literal_type type)
                : node(selection, node_type::EXPR_LITERAL), type(type) {}

            literal_type type;
        };

        struct expr_unary : node {
            expr_unary(const core::lisel& selection, node_id operand, const core::token& opr, const bool post)
                : node(selection, node_type::EXPR_UNARY), operand(operand), opr(opr), post(post) {}

            node_id operand;
            core::token opr;
            bool post;
        };

        struct expr_binary : node {
            expr_binary(const core::lisel& selection, node_id first, node_id second, const core::token& opr)
                : node(selection, node_type::EXPR_BINARY), first(first), second(second), opr(opr) {}

            node_id first;
            node_id second;
            core::token opr;
        };
        
        struct expr_ternary : node {
            expr_ternary(const core::lisel& selection, node_id first, node_id second, node_id third)
                : node(selection, node_type::EXPR_TERNARY), first(first), second(second), third(third) {}

            node_id first;
            node_id second;
            node_id third;
        };
        
        struct expr_parameter : node {
            expr_parameter(const core::lisel& selection, node_id name, node_id default_value, node_id type)
                : node(selection, node_type::EXPR_PARAMETER), name(name), default_value(default_value), type(type) {}

            node_id name;
            node_id default_value;
            node_id type;
        };

        struct expr_function : node {
            expr_function(const core::lisel& selection, std::vector<node_id>&& parameter_list, node_id body, node_id return_type)
                : node(selection, node_type::EXPR_FUNCTION), parameter_list(std::move(parameter_list)), body(body), return_type(return_type) {}

            std::vector<node_id> parameter_list;
            node_id body;
            node_id return_type;
        };

        struct expr_call : node {
            expr_call(const core::lisel& selection, node_id callee, std::vector<node_id>&& argument_list)
                : node(selection, node_type::EXPR_CALL), callee(callee), argument_list(std::move(argument_list)) {}

            node_id callee;
            std::vector<node_id> argument_list;
        };

        struct expr_local_declaration : node {
            expr_local_declaration(const core::lisel& selection, node_id name, node_id value, node_id type)
                : node(selection, node_type::EXPR_LOCAL_DECLARATION), name(name), value(value), type(type) {}
            
            node_id name; // Use up_expr instead of expr_identifier since variables can be declared within scopes.
            node_id value;
            node_id type;
        };
        
        struct stmt_none : node {
            stmt_none(const core::lisel& selection)
                : node(selection, node_type::STMT_NONE) {}
        };

        struct stmt_invalid : node {
            stmt_invalid(const core::lisel& selection)
                : node(selection, node_type::STMT_INVALID) {}
        };

        struct stmt_if : node {
            stmt_if(const core::lisel& selection, node_id condition, node_id consequent, node_id alternate)
                : node(selection, node_type::STMT_IF), condition(condition), consequent(consequent), alternate(alternate) {}

            node_id condition;
            node_id consequent;
            node_id alternate;
        };
        
        struct stmt_while : node {
            stmt_while(const core::lisel& selection, node_id condition, node_id consequent, node_id alternate)
                : node(selection, node_type::STMT_WHILE), condition(condition), consequent(consequent), alternate(alternate) {}

            node_id condition;
            node_id consequent;
            node_id alternate; // yes, we have else clauses in while loops
        };
        
        struct stmt_return : node {
            stmt_return(const core::lisel& selection, node_id expression)
                : node(selection, node_type::STMT_RETURN), expression(expression) {}
            
            node_id expression;
        };

        struct stmt_wrapper : node {
            stmt_wrapper(node_id expression)
                : node(core::lisel(0,0), node_type::STMT_WRAPPER), expression(expression) {}

            node_id expression;
        };

        struct stmt_body : node {
            stmt_body(const core::lisel& selection, std::vector<node_id>&& statement_list)
                : node(selection, node_type::STMT_BODY), statement_list(std::move(statement_list)) {}

            std::vector<node_id> statement_list;
        };

        struct stmt_break : node {
            stmt_break(const core::lisel& selection)
                : node(selection, node_type::STMT_BREAK) {}
        };

        struct stmt_continue : node {
            stmt_continue(const core::lisel& selection)
                : node(selection, node_type::STMT_CONTINUE) {}
        };

        struct s_stmt_use : node {
            s_stmt_use(const core::lisel& selection, node_id path)
                : node(selection, node_type::S_STMT_USE), path(path) {}

            // The parser must ensure that this literal is a string. Store as node_id into arena.
            node_id path;
        };

        struct s_stmt_scoped_body : node {
            s_stmt_scoped_body(const core::lisel& selection, std::vector<node_id>&& statement_list)
                : node(selection, node_type::S_STMT_SCOPED_BODY), statement_list(std::move(statement_list)) {}

            std::vector<node_id> statement_list;
        };

        struct s_stmt_namespace : node {
            s_stmt_namespace(const core::lisel& selection, node_id name, node_id content)
                : node(selection, node_type::S_STMT_NAMESPACE), name(name), content(content) {}

            node_id name;
            node_id content;
        };

        struct s_stmt_declaration : node {
            s_stmt_declaration(const core::lisel& selection, node_id name, node_id value, node_id type)
                : node(selection, node_type::S_STMT_DECLARATION), name(name), value(value), type(type) {}
            
            node_id name;
            node_id value;
            node_id type;
        };

        struct s_stmt_invalid : node {
            s_stmt_invalid(const core::lisel& selection)
                : node(selection, node_type::S_STMT_INVALID) {}
        };

        struct arena_node {
            template <typename T>
            arena_node(T&& node)
                : _raw(std::forward<T>(node)) {}

            std::variant<
                ast_root,

                expr_none,
                expr_invalid,
                expr_type,
                expr_identifier,
                expr_literal,
                expr_unary,
                expr_binary,
                expr_ternary,
                expr_parameter,
                expr_function,
                expr_call,
                expr_local_declaration,

                stmt_none,
                stmt_invalid,
                stmt_if,
                stmt_while,
                stmt_return,
                stmt_wrapper,
                stmt_body,
                stmt_break,
                stmt_continue,

                s_stmt_use,
                s_stmt_scoped_body,
                s_stmt_namespace,
                s_stmt_declaration,
                s_stmt_invalid
            > _raw;
        };

        struct ast_arena {
            // !!EXPECTED BEHAVIOR!! - 0th index is the root!!
            std::vector<arena_node> node_list = {};

            template <typename T>
            inline node_id insert(T&& node) {
                node_list.emplace_back(std::forward<T>(node));
                return node_list.size() - 1;
            }

            template <typename T>
            // Insert a node into the arena and get its type.
            inline T& static_insert(T&& node) {
                node_list.emplace_back(std::forward<T>(node));
                return std::get<T>(node_list.back()._raw);
            }

            
            template <typename T>
            inline T& get(const node_id id) {
                return std::get<T>(node_list[id]._raw);
            }

            template <typename T>
            inline const T& get(const node_id id) const {
                return std::get<T>(node_list[id]._raw);
            }


            inline node* get_ptr(const node_id id) {
                return std::visit([](auto& n) { return (node*)(&n); }, node_list[id]._raw);
            }

            inline const node* get_ptr(const node_id id) const {
                return std::visit([](auto& n) { return (const node*)(&n); }, node_list[id]._raw);
            }

            
            template <typename T>
            inline T& get_as(const node_id id) {
                return std::get<T>(node_list[id]._raw);
            }

            template <typename T>
            inline const T& get_as(const node_id id) const {
                return std::get<T>(node_list[id]._raw);
            }

            void pretty_debug(const liprocess& process, const node_id id, std::string& buffer, uint8_t indent = 0);
            // void pretty_debug(const liprocess& process, const node* n, std::string& buffer, uint8_t indent = 0) const;
        };
    }
}