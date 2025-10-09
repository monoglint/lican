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

            EXPR_PROPERTY,
            EXPR_METHOD,
            EXPR_OPERATOR,
            EXPR_INITIALIZER_SET,
            EXPR_CONSTRUCTOR,
            EXPR_DESTRUCTOR,

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
            enum class e_reference_type : uint8_t {
                NONE,
                LVALUE,
                RVALUE
            };

            expr_type(const core::lisel& selection, const t_node_id source, t_node_list&& argument_list, const bool is_const, const bool is_pointer, const e_reference_type reference_type)
                : node(selection, node_type::EXPR_TYPE), source(source), argument_list(std::move(argument_list)), is_const(is_const), is_pointer(is_pointer), reference_type(reference_type) {}
            
            // expr_identifier | binary (scope_resolution)
            t_node_id source; 
            t_node_list argument_list;
            
            bool is_const;
            bool is_pointer;
            e_reference_type reference_type;
        };

        // Get identifier contents by observing its selection in the source code.
        struct expr_identifier : node {
            expr_identifier(const core::lisel& selection)
                : node(selection, node_type::EXPR_IDENTIFIER) {}
        };

        struct expr_literal : node {
            enum class e_literal_type : uint8_t {
                FLOAT,
                INT,
                STRING,
                CHAR,
                BOOL,
                NIL,
            };

            expr_literal(const core::lisel& selection, const e_literal_type literal_type)
                : node(selection, node_type::EXPR_LITERAL), literal_type(literal_type) {}

            e_literal_type literal_type;
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

            struct expr_property : node {
                expr_property(const core::lisel& selection, const t_node_id name, const t_node_id value_type, const t_node_id default_value, const bool is_private)
                    : node(selection, node_type::EXPR_PROPERTY), name(name), value_type(value_type), default_value(default_value), is_private(is_private) {}

                t_node_id name;
                t_node_id value_type;
                t_node_id default_value;

                bool is_private;
            };

            struct expr_method : node {
                expr_method(const core::lisel& selection, const t_node_id name, const t_node_id function, const bool is_private, const bool is_const)
                    : node(selection, node_type::EXPR_METHOD), name(name), function(function), is_private(is_private), is_const(is_const) {}

                t_node_id name;
                t_node_id function;

                bool is_private;
                bool is_const;
            };
            
            struct expr_operator : node {
                expr_operator(const core::lisel& selection, const core::token_type opr, const t_node_id function, const bool is_const)
                    : node(selection, node_type::EXPR_OPERATOR), opr(opr), function(function), is_const(is_const) {}
                    
                core::token_type opr;
                t_node_id function;
                bool is_const;
            };

            struct expr_initializer_set : node {
                expr_initializer_set(const core::lisel& selection, const t_node_id property_name, const t_node_id value)
                    : node(selection, node_type::EXPR_INITIALIZER_SET), property_name(property_name), value(value) {}

                t_node_id property_name; // identifier
                t_node_id value;
            };

            struct expr_constructor : node {
                expr_constructor(const core::lisel& selection, const t_node_id name, const t_node_id function, t_node_list&& initializer_list)
                    : node(selection, node_type::EXPR_CONSTRUCTOR), name(name), function(function), initializer_list(std::move(initializer_list)) {}
                
                t_node_id name;
                t_node_id function;
                t_node_list initializer_list; // expr_initializer_set
            };

            struct expr_destructor : node {
                expr_destructor(const core::lisel& selection, const t_node_id body)
                    : node(selection, node_type::EXPR_DESTRUCTOR), body(body) {}

                t_node_id body;
            };

        struct item_struct_declaration : node {
            item_struct_declaration(const core::lisel& selection, const t_node_id name, t_node_list&& template_parameter_list, t_node_list&& member_list)
                : node(selection, node_type::ITEM_STRUCT_DECLARATION), name(name), template_parameter_list(std::move(template_parameter_list)), member_list(std::move(member_list)) {}

            t_node_id name;

            t_node_list template_parameter_list;
            t_node_list member_list; // struct_member | operator_overload | constructor | destructor
        };

        
        struct expr_enum_set : node {
            expr_enum_set(const core::lisel& selection, const t_node_id name, const t_node_id value)
                : node(selection, node_type::EXPR_ENUM_SET), name(name), value(value) {}

            t_node_id name;
            t_node_id value;
        };
        
        struct item_enum : node {
            item_enum(const core::lisel& selection, const t_node_id name, t_node_list&& set_list)
                : node(selection, node_type::ITEM_ENUM), name(name), set_list(std::move(set_list)) {}

            t_node_id name;
            t_node_list set_list; // expr_enum_set | expr_none
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

                expr_property,
                expr_method,
                expr_operator,
                expr_initializer_set,
                expr_constructor,
                expr_destructor,
                item_struct_declaration,
                expr_enum_set,
                item_enum,
                
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