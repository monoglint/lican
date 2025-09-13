// A tree walker that generates a symbol table and checks it as it does so.

#include "core.hpp"
#include "ast.hpp"
#include "symbol.hpp"

struct semantic_state {
    semantic_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), ast(*(std::any_cast<const std::unique_ptr<core::ast::ast_root>&>(process.file_list[file_id].dump_ast_root))) {}

    core::liprocess& process;

    const core::t_file_id file_id;
    const core::ast::ast_root& ast;

    // Equiv to "root" in the parser. Must be initialized and used here since we're walking and not generating.
    core::sym::p_crate focused_symbol = std::make_unique<core::sym::crate_root>();
};

static bool walk_stmt_declaration(semantic_state& state, core::ast::stmt_declaration&& node) {
    // Check if the value of the node is a function.

    if (node.value->type == core::ast::node_type::EXPR_FUNCTION) {
        // build(function, parameter_types, return_type)
        
        return true;
    }

    // build(declaration, type)

    return true;
}

static bool walk_statement(semantic_state& state, core::ast::p_stmt& statement) {
    switch (statement->type) {
        case core::ast::node_type::STMT_DECLARATION:
            return dynamic_cast<core::ast::stmt_declaration*>(statement.get());
    }
    return false;
}

bool core::frontend::semantic_analyze(liprocess& process, const t_file_id file_id) {
    semantic_state state(process, file_id);
    
    return true;
}