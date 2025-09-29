// A tree walker that generates a symbol table and checks it as it does so.

#include "core.hpp"
#include "ast.hpp"
#include "symbol.hpp"

struct semantic_state {
    semantic_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), ast(*(std::any_cast<const std::unique_ptr<core::ast::ast_root>&>(process.file_list[file_id].dump_ast_root))) {

            // Initialize the root to the standard system.
            root.symbol_list.emplace("u8", core::sym::type_primitive(1));
            root.symbol_list.emplace("i8", core::sym::type_primitive(1));

            root.symbol_list.emplace("u16", core::sym::type_primitive(2));
            root.symbol_list.emplace("i16", core::sym::type_primitive(2));

            root.symbol_list.emplace("u32", core::sym::type_primitive(4));
            root.symbol_list.emplace("i32", core::sym::type_primitive(4));

            root.symbol_list.emplace("u64", core::sym::type_primitive(8));
            root.symbol_list.emplace("i64", core::sym::type_primitive(8));

            root.symbol_list.emplace("f32", core::sym::type_primitive(4));
            root.symbol_list.emplace("f64", core::sym::type_primitive(8));

            root.symbol_list.emplace("void", core::sym::type_primitive(0));

            root.symbol_list.emplace("bool", core::sym::type_primitive(1));
        }

    core::liprocess& process;

    const core::t_file_id file_id;
    const core::ast::ast_root& ast;

    core::sym::crt_root root; // auto initialized
    core::sym::crate& focused_symbol = root;

    // licantype
    core::sym::p_symbol SYM_INVALID = std::make_unique<core::sym::sym_invalid>(); // this is const safe since there's nothing to really do with it
    const core::sym::p_type LT_UNRESOLVED = std::make_unique<core::sym::type_primitive>(0);

    // Ensure that node is already guaranteed to be an identifier. This function is used in functions where a node might be casted differently based on runtime conditions.
    inline std::string read_identifier(const core::ast::expr* node) {
        return process.sub_source_code(static_cast<const core::ast::expr_identifier*>(node)->selection); 
    }

    core::sym::p_symbol& find_symbol(const core::ast::expr* resolution_node) {
        // Expect type in global scope

        switch (resolution_node->type) {
            case core::ast::node_type::EXPR_IDENTIFIER: {
                const std::string child_name = read_identifier(resolution_node);

                if (root.symbol_list.find(child_name) == root.symbol_list.end()) {
                    process.add_log(core::lilog::log_level::ERROR, resolution_node->selection, "'" + child_name + "' was not declared in this scope.");
                    return SYM_INVALID;
                }

                return root.symbol_list.at(child_name);
            }

            case core::ast::node_type::EXPR_BINARY: {
                const auto* binary_node = static_cast<const core::ast::expr_binary*>(resolution_node);
                auto* parent_crate = static_cast<core::sym::crate*>(find_symbol(binary_node->first.get()).get());

                const std::string child_name = read_identifier(binary_node->second.get());

                if (parent_crate->symbol_list.find(child_name) == root.symbol_list.end()) {
                    process.add_log(core::lilog::log_level::ERROR, resolution_node->selection, "'" + child_name + "' was not declared in this scope.");
                    return SYM_INVALID;
                }

                return parent_crate->symbol_list.at(child_name);
            }

            default:
                process.add_log(core::lilog::log_level::ERROR, resolution_node->selection, "Internal compiler error: Attempted to find a symbol with an invalid node.");

        }

    }

    core::sym::type_alias make_type_from_node(const core::ast::expr* node) {
        if (node->type == core::ast::node_type::EXPR_NONE) {
            return core::sym::type_alias(LT_UNRESOLVED);
        }

        const auto* type_node = static_cast<const core::ast::expr_type*>(node);
        const core::sym::p_symbol& source = find_symbol(type_node->source.get());

        std::vector<core::sym::type_alias> argument_list = {};

        for (const std::unique_ptr<core::ast::expr_type>& arg_node : type_node->argument_list) {
            auto arg_sym = make_type_from_node(static_cast<core::ast::expr*>(arg_node.get()));
            argument_list.emplace_back(arg_sym);
        }

        return core::sym::type_alias(source, argument_list, type_node->is_mutable, type_node->is_reference);
    }
};

static bool walk_expression(semantic_state& state, const core::ast::expr* node) {
    switch (node->type) {
        case core::ast::node_type::EXPR_LOCAL_DECLARATION:
    }

    return false;
}

static std::vector<core::sym::type_alias> get_parameter_list(const core::ast::expr_function* node) {
    for (const std::unique_ptr<core::ast::expr_parameter>& param : node->parameter_list) {

    }
}

static bool walk_s_stmt_declaration(semantic_state& state, const core::ast::s_stmt_declaration* node) {
    // Check if the value of the node is a function.

    if (node->value->type == core::ast::node_type::EXPR_FUNCTION) {
        const auto* function_node = static_cast<core::ast::expr_function*>(node->value.get());
        const std::vector<core::sym::type_alias> parameter_list = get_parameter_list(function_node);
        const core::sym::type_alias return_type = state.make_type_from_node(function_node->return_type.get());

        state.focused_symbol.symbol_list.emplace(state.process.sub_source_code(node->name->selection), std::make_unique<core::sym::sym_function>(return_type, parameter_list));

        // build(function, parameter_types, return_type)
        
        return true;
    }

    // build(declaration, type)

    return true;
    
    return true;
}

static bool walk_s_statement(semantic_state& state, core::ast::p_s_stmt statement) {
    switch (statement->type) {       
        case core::ast::node_type::S_STMT_DECLARATION:
            return walk_s_stmt_declaration(state, static_cast<const core::ast::s_stmt_declaration*>(statement.get()));
    }
    return false;
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);
    
    return true;
}