/*

====================================================

Contains all of the AST node declarations. Used by the frontend and read by the backend.

====================================================

*/

#pragma once

#include <cstdint>
#include <memory>
#include <variant>

#include "util.hh"
#include "core.hh"
#include "token.hh"

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

            EXPR_CALL,

            STMT_NONE,
            STMT_INVALID,
            STMT_IF,
            STMT_WHILE,
            STMT_RETURN,
            ITEM_BODY,
            STMT_BREAK,
            STMT_CONTINUE,

            ITEM_USE,
            ITEM_MODULE,
            VARIANT_DECLARATION,
            ITEM_TYPE_DECLARATION,

            EXPR_STRUCT_MEMBER,
            EXPR_STRUCT_OPERATOR,
            EXPR_STRUCT_LIFECYCLE_MEMBER,
            ITEM_STRUCT_DECLARATION,

            EXPR_ENUM_SET,
            ITEM_ENUM,

            ITEM_INVALID,
        };

        using t_node_id = size_t;
        using t_node_list = std::vector<t_node_id>;
        
        struct node {
            node(const core::lisel& selection, const node_type type)
                : selection(selection), type(type) {}

            node_type type;
            core::lisel selection;
        };

        struct ast_root : node {
            ast_root()
                : node(core::lisel(0, 0), node_type::ROOT) {}

            t_node_list item_list;
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
            expr_type(const core::lisel& selection, const t_node_id source, t_node_list&& argument_list, const bool is_mutable, const bool is_pointer)
                : node(selection, node_type::EXPR_TYPE), source(source), argument_list(std::move(argument_list)), is_mutable(is_mutable), is_pointer(is_pointer) {}
            
            t_node_id source; // Identifier or scope resolution
            t_node_list argument_list;
            
            bool is_mutable;
            bool is_pointer;
        };

        // Get identifier contents by observing its selection in the source code.
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

            expr_literal(const core::lisel& selection, const literal_type value_type)
                : node(selection, node_type::EXPR_LITERAL), value_type(value_type) {}

            literal_type value_type;
        };

        struct expr_unary : node {
            expr_unary(const core::lisel& selection, t_node_id operand, const core::token& opr, const bool post)
                : node(selection, node_type::EXPR_UNARY), operand(operand), opr(opr), post(post) {}

            t_node_id operand;
            core::token opr;
            bool post;
        };

        struct expr_binary : node {
            expr_binary(const core::lisel& selection, t_node_id first, t_node_id second, const core::token& opr)
                : node(selection, node_type::EXPR_BINARY), first(first), second(second), opr(opr) {}

            t_node_id first;
            t_node_id second;
            core::token opr;
        };
        
        struct expr_ternary : node {
            expr_ternary(const core::lisel& selection, t_node_id first, t_node_id second, t_node_id third)
                : node(selection, node_type::EXPR_TERNARY), first(first), second(second), third(third) {}

            t_node_id first;
            t_node_id second;
            t_node_id third;
        };
        
        struct expr_parameter : node {
            expr_parameter(const core::lisel& selection, t_node_id name, t_node_id default_value, t_node_id value_type)
                : node(selection, node_type::EXPR_PARAMETER), name(name), default_value(default_value), value_type(value_type) {}

            t_node_id name;
            t_node_id default_value;
            t_node_id value_type;
        };

        struct expr_function : node {
            expr_function(const core::lisel& selection, t_node_list&& template_parameter_list, t_node_list&& parameter_list, t_node_id body, t_node_id return_type)
                : node(selection, node_type::EXPR_FUNCTION), template_parameter_list(std::move(template_parameter_list)), parameter_list(std::move(parameter_list)), body(body), return_type(return_type) {}

            t_node_list template_parameter_list;
            t_node_list parameter_list;
            t_node_id body;
            t_node_id return_type;
        };

        struct expr_call : node {
            expr_call(const core::lisel& selection, t_node_id callee, t_node_list&& template_argument_list, t_node_list&& argument_list)
                : node(selection, node_type::EXPR_CALL), callee(callee), template_argument_list(std::move(template_argument_list)), argument_list(std::move(argument_list)) {}

            t_node_id callee;
            t_node_list template_argument_list;
            t_node_list argument_list;
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
            stmt_if(const core::lisel& selection, t_node_id condition, t_node_id consequent, t_node_id alternate)
                : node(selection, node_type::STMT_IF), condition(condition), consequent(consequent), alternate(alternate) {}

            t_node_id condition;
            t_node_id consequent;
            t_node_id alternate;
        };
        
        struct stmt_while : node {
            stmt_while(const core::lisel& selection, t_node_id condition, t_node_id consequent, t_node_id alternate)
                : node(selection, node_type::STMT_WHILE), condition(condition), consequent(consequent), alternate(alternate) {}

            t_node_id condition;
            t_node_id consequent;
            t_node_id alternate; // yes, we have else clauses in while loops
        };
        
        struct stmt_return : node {
            stmt_return(const core::lisel& selection, t_node_id expression)
                : node(selection, node_type::STMT_RETURN), expression(expression) {}
            
            t_node_id expression;
        };

        struct item_body : node {
            item_body(const core::lisel& selection, t_node_list&& item_list)
                : node(selection, node_type::ITEM_BODY), item_list(std::move(item_list)) {}

            t_node_list item_list;
        };

        struct stmt_break : node {
            stmt_break(const core::lisel& selection)
                : node(selection, node_type::STMT_BREAK) {}
        };

        struct stmt_continue : node {
            stmt_continue(const core::lisel& selection)
                : node(selection, node_type::STMT_CONTINUE) {}
        };

        struct item_use : node {
            item_use(const core::lisel& selection, t_node_id path)
                : node(selection, node_type::ITEM_USE), path(path) {}

            // The parser must ensure that this literal is a string. Store as t_node_id into arena.
            t_node_id path;
        };

        struct item_module : node {
            item_module(const core::lisel& selection, t_node_id name, t_node_id content)
                : node(selection, node_type::ITEM_MODULE), name(name), content(content) {}

            t_node_id name;
            t_node_id content;
        };

        struct variant_declaration : node {
            variant_declaration(const core::lisel& selection, t_node_id name, t_node_id value, t_node_id value_type)
                : node(selection, node_type::VARIANT_DECLARATION), name(name), value(value), value_type(value_type) {}
            
            t_node_id name;
            t_node_id value;
            t_node_id value_type;
        };

        struct item_type_declaration : node {
            item_type_declaration(const core::lisel& selection, t_node_id name, t_node_id type_value, t_node_list&& parameter_list)
                : node(selection, node_type::ITEM_TYPE_DECLARATION), name(name), type_value(type_value), parameter_list(std::move(parameter_list)) {}
            
            t_node_id name;
            t_node_id type_value;
            t_node_list parameter_list; // typedec resizable_with_array_with_t<T> = resizable<array<T>>
        };

            // Properties & methods
            struct expr_struct_member : node {
                expr_struct_member(const core::lisel& selection, const t_node_id name, const t_node_id value_type, const t_node_id default_value, const bool is_internal)
                    : node(selection, node_type::EXPR_STRUCT_MEMBER), name(name), value_type(value_type), default_value(default_value), is_internal(is_internal) {}

                t_node_id name;
                t_node_id value_type;
                t_node_id default_value;

                bool is_internal;
            };
            
            struct expr_struct_operator : node {
                expr_struct_operator(const core::lisel& selection, const core::token_type opr, const t_node_id function)
                    : node(selection, node_type::EXPR_STRUCT_OPERATOR), opr(opr), function(function) {}
                    
                core::token_type opr;
                t_node_id function; // parser should syntactically ensure that the parameters to the function are semantically accurate
            };

            // Lifecycle specific methods
            struct expr_struct_lifecycle_member : node {
                enum class lifecycle_member_type {
                    CONSTRUCTOR,
                    COPY_CONSTRUCTOR,
                    DESTRUCTOR,
                };

                expr_struct_lifecycle_member(const core::lisel& selection, const t_node_id name, const lifecycle_member_type lifecycle_type)
                    : node(selection, node_type::EXPR_STRUCT_LIFECYCLE_MEMBER), name(name), lifecycle_type(lifecycle_type) {}

                t_node_id name;
                lifecycle_member_type lifecycle_type;
            };

            struct item_struct_declaration : node {
                item_struct_declaration(const core::lisel& selection, const t_node_id name, t_node_list&& template_parameter_list, t_node_list&& member_list)
                    : node(selection, node_type::ITEM_STRUCT_DECLARATION), name(name), template_parameter_list(std::move(template_parameter_list)), member_list(std::move(member_list)) {}

                t_node_id name;

                t_node_list template_parameter_list;
                t_node_list member_list; // struct_member, operator_overload, or struct_lifecycle_member
            };

        
        struct expr_enum_set : node {
            expr_enum_set(const core::lisel& selection, const t_node_id name, const t_node_id custom_value)
                : node(selection, node_type::EXPR_ENUM_SET), name(name), custom_value(custom_value) {}

            t_node_id name;
            t_node_id custom_value;
        };
        
        struct item_enum : node {
            item_enum(const core::lisel& selection, const t_node_id name, t_node_list&& set_list)
                : node(selection, node_type::EXPR_ENUM_SET), name(name), set_list(std::move(set_list)) {}

            t_node_id name;
            t_node_list set_list;
        };

        struct item_invalid : node {
            item_invalid(const core::lisel& selection)
                : node(selection, node_type::ITEM_INVALID) {}
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

                stmt_none,
                stmt_invalid,
                stmt_if,
                stmt_while,
                stmt_return,
                item_body,
                stmt_break,
                stmt_continue,

                item_use,
                item_module,
                variant_declaration,
                item_type_declaration,

                expr_struct_member,
                expr_struct_operator,
                expr_struct_lifecycle_member,
                item_struct_declaration,
                
                item_invalid
            > _raw;
        };

        struct ast_arena {
            // !!EXPECTED BEHAVIOR!! - 0th index is the root!!
            std::vector<arena_node> node_list = {};

            template <typename T>
            inline t_node_id insert(T&& node) {
                node_list.push_back(std::forward<T>(node));
                return node_list.size() - 1;
            }

            template <typename T>
            // Insert a node into the arena and get its type.
            inline T& static_insert(T&& node) {
                node_list.push_back(std::forward<T>(node));
                return std::get<T>(node_list.back()._raw);
            }

            
            template <typename T>
            inline T& get(const t_node_id id) {
                return std::get<T>(node_list[id]._raw);
            }

            template <typename T>
            inline const T& get(const t_node_id id) const {
                return std::get<T>(node_list[id]._raw);
            }


            // Not safe for long-term pointer usage. Only use for direct modification and disposal of the given pointer.
            inline node* get_base_ptr(const t_node_id id) {
                return std::visit([](auto& n) { return (node*)(&n); }, node_list[id]._raw);
            }

            // Not safe for long-term pointer usage. Only use for direct access and disposal of the given pointer.
            inline const node* get_base_ptr(const t_node_id id) const {
                return std::visit([](auto& n) { return (const node*)(&n); }, node_list[id]._raw);
            }

            
            template <typename T>
            inline T& get_as(const t_node_id id) {
                return std::get<T>(node_list[id]._raw);
            }

            template <typename T>
            inline const T& get_as(const t_node_id id) const {
                return std::get<T>(node_list[id]._raw);
            }

            void pretty_debug(const liprocess& process, const t_node_id id, std::string& buffer, uint8_t indent = 0);
            bool is_expression_wrappable(const t_node_id id);
        };
    }
}