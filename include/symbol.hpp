#pragma once

#include <memory>
#include <unordered_map>
#include <optional>

namespace core {
    namespace sym {
        enum class symbol_type {
            ROOT,
            INVALID,
            
            ALIAS,
            PRIMITIVE,
            ARRAY,

            VARIABLE_DECLARATION,
            TYPE_DECLARATION,
            NAMESPACE,
        };

        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            virtual ~symbol() = default;
            symbol_type type;
        };

        using up_symbol = std::unique_ptr<symbol>;

        // A crate is a symbol that is guaranteed to hold other symbols.
        struct crate : symbol {
            crate(const symbol_type type)
                : symbol(type) {}

            virtual ~crate() = default;

            std::unordered_map<std::string, up_symbol> symbol_list;
        };

        // A symbol that is found directly within a crate.
        struct branch : symbol {
            branch(const std::string& name, const symbol_type type)
                : symbol(type), name(name) {}

            virtual ~branch() = default;

            mutable std::string name;
        };

        struct sym_invalid : symbol {
            sym_invalid()
                : symbol(symbol_type::INVALID) {}
        };

        // A type is a symbol that is either a primitive, a user defined type, or a reference to another type with arguments and attachments.
        struct type : symbol {
            type(const symbol_type type)
                : symbol(type) {}

            virtual ~type() = default;
        };

        using up_crate = std::unique_ptr<crate>;
        using up_type = std::unique_ptr<type>;
        using up_branch = std::unique_ptr<branch>;

        struct type_alias : type {
            type_alias(type* source, std::vector<std::reference_wrapper<type_alias>>&& argument_list = {}, const bool is_mutable = false, const bool is_reference = false)
                : type(symbol_type::ALIAS), source(source), argument_list(std::move(argument_list)), is_mutable(is_mutable), is_reference(is_reference) {}
    
            type* source;
            std::vector<std::reference_wrapper<type_alias>> argument_list;
            bool is_mutable;
            bool is_reference; 
        };


        struct type_primitive : type {
            type_primitive(size_t size)
                : type(symbol_type::PRIMITIVE), size(size) {}

            size_t size;
        };

        struct type_array : type {
            type_array(const size_t size, up_type& content_type)
                : type(symbol_type::ARRAY), size(size), content_type(std::move(content_type)) {}

            size_t size;
            up_type content_type;
        };

        struct sym_variable_declaration : branch {
            sym_variable_declaration(const std::string& name, const type_alias& type)
                : branch(name, symbol_type::VARIABLE_DECLARATION), type(std::move(type)) {}

            type_alias type;
        };

        struct sym_type_declaration : branch {
            sym_type_declaration(const std::string& name, const type_alias& type)
                : branch(name, symbol_type::TYPE_DECLARATION), type(std::move(type)) {}

            type_alias type;
        };

        struct crt_root : crate {
            crt_root()
                : crate(symbol_type::ROOT) {}
        };

        struct crt_namespace : crate, branch {
            crt_namespace(const std::string& name)
                : crate(symbol_type::NAMESPACE), branch(name, symbol_type::NAMESPACE) {}
        };
    }
}
