#include "core.hpp"
#include "ast.hpp"

struct generate_state {
    generate_state(core::liprocess& process, const core::t_file_id file_id)
        : process(process), file_id(file_id), ast(*(std::any_cast<const std::unique_ptr<core::ast::ast_root>&>(process.file_list[file_id].dump_ast_arena))) {}

    core::liprocess& process;

    const core::t_file_id file_id;
    const core::ast::ast_root& ast;
};