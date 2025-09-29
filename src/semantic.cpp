// A tree walker that generates a symbol table and checks it as it does so.

#include "core.hpp"
#include "ast.hpp"
#include "symbol.hpp"

template <typename T, typename V>
static T& deref_to(V& smart_pointer) {
    return *static_cast<T*>(smart_pointer.get());
}

struct semantic_state {
    semantic_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), ast(*(std::any_cast<const std::shared_ptr<core::ast::ast_root>&>(process.file_list[file_id].dump_ast_root))) {

            // Initialize the root to the standard system.
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("u8", 1));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("i8", 1));

            // emplace_symbol(std::make_unique<core::sym::type_primitive>("u16", 2));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("i16", 2));

            // emplace_symbol(std::make_unique<core::sym::type_primitive>("u32", 4));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("i32", 4));

            // emplace_symbol(std::make_unique<core::sym::type_primitive>("u64", 8));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("i64", 8));

            // emplace_symbol(std::make_unique<core::sym::type_primitive>("f32", 4));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("f64", 8));

            // emplace_symbol(std::make_unique<core::sym::type_primitive>("void", 0));
            // emplace_symbol(std::make_unique<core::sym::type_primitive>("bool", 1));
        }

    // Constant types
    core::sym::up_symbol TYPE_INVALID = std::make_unique<core::sym::type_primitive>(0);

    core::liprocess& process;

    const core::t_file_id file_id;
    core::ast::ast_root& ast;

    core::sym::crt_root root; // auto initialized
    core::sym::crate& focused_symbol = root;

    inline void emplace_symbol(core::sym::up_branch&& symbol) {
        focused_symbol.symbol_list[symbol->name] = std::move(symbol);
    }

    // Ensure that node is already guaranteed to be an identifier. This function is used in functions where a node might be casted differently based on runtime conditions.
    inline std::string read_identifier(const core::ast::up_expr& node) const {
        return process.sub_source_code(deref_to<core::ast::expr_identifier>(node).selection);
    }

    core::sym::up_symbol& find_symbol(const core::ast::up_expr& node) {
        core::sym::crate* parent_crate;
        std::string symbol_name;
        
        if (node->type == core::ast::node_type::EXPR_IDENTIFIER) {
            parent_crate = &root;
            symbol_name = read_identifier(node);
        } 
        else if (node->type == core::ast::node_type::EXPR_BINARY) {
            auto& binary_node = deref_to<core::ast::expr_binary>(node);
            
            if (binary_node.opr.type != core::token_type::DOUBLE_COLON) {
                process.add_log(core::lilog::log_level::ERROR, node->selection, "Incorrect expression for finding a symbol.");

                return TYPE_INVALID;
            }

            parent_crate = (core::sym::crate*)(&find_symbol(binary_node.first));
            symbol_name = read_identifier(binary_node.second);
        }
        else {
            process.add_log(core::lilog::log_level::ERROR, node->selection, "Incorrect expression for finding a symbol.");

            return TYPE_INVALID;
        }
        
        if (parent_crate->symbol_list.find(symbol_name) == parent_crate->symbol_list.end()) {
            process.add_log(core::lilog::log_level::ERROR, node->selection, "'" + symbol_name + "' was not declared in this scope.");
            return TYPE_INVALID;
        }

        return parent_crate->symbol_list.at(symbol_name);
    }

    core::sym::type_alias make_type_from_node(const core::ast::expr_type& node) {
        auto* source = (core::sym::type*)find_symbol(node.source).get();

        std::vector<std::reference_wrapper<core::sym::type_alias>> argument_list = {};


        return core::sym::type_alias(source, std::move(argument_list), node.is_mutable, node.is_reference);
    }
};

static bool walk_expression(semantic_state& state, const core::ast::up_expr node) {
    switch (node->type) {
        case core::ast::node_type::EXPR_LOCAL_DECLARATION:
            break;
    }

    return false;
}

static std::vector<core::sym::type_alias> get_parameter_list(const core::ast::expr_function& node) {
    for (const std::unique_ptr<core::ast::expr_parameter>& param : node.parameter_list) {

    }

    return {};
}

static bool walk_s_stmt_declaration(semantic_state& state, const core::ast::s_stmt_declaration& node) {
    if (node.value->type == core::ast::node_type::EXPR_FUNCTION) {
        std::cout << "Declared fn\n";
        
        return true;
    }
    
    return true;
}

static bool walk_s_statement(semantic_state& state, core::ast::up_s_stmt& statement) {
    switch (statement->type) {       
        case core::ast::node_type::S_STMT_DECLARATION:
            return walk_s_stmt_declaration(state, deref_to<const core::ast::s_stmt_declaration>(statement));
    }
    return false;
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);

    bool is_success = true;

    for (core::ast::up_s_stmt& statement : state.ast.statement_list) {
        if (!walk_s_statement(state, statement))
            is_success = false;
    }
    
    return true;
}