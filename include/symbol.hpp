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
            STRUCT,
            ARRAY,

            FUNCTION,

            NAMESPACE,
        };

        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            virtual ~symbol() = default;

            const symbol_type type;
        };

        using p_symbol = std::unique_ptr<symbol>;

        // A crate is a symbol that is guaranteed to hold other symbols.
        struct crate : symbol {
            crate(const symbol_type type)
                : symbol(type) {}

            virtual ~crate() = default;

            std::unordered_map<std::string, p_symbol> symbol_list;
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

        using p_crate = std::unique_ptr<crate>;
        using p_type = std::unique_ptr<type>;

        struct type_alias : type {
            type_alias(const p_type& source, std::vector<type_alias>& argument_list, const bool is_mutable = false, const bool is_reference = false)
                : type(symbol_type::ALIAS), source(source), argument_list(std::move(argument_list)), is_mutable(is_mutable), is_reference(is_reference) {}
            
            // chose a separate constructor rather than an optional to simplify the parameters
            type_alias(const p_type& source, const bool is_mutable = false, const bool is_reference = false)
                : type(symbol_type::ALIAS), source(source), argument_list(std::nullopt), is_mutable(is_mutable), is_reference(is_reference) {}
            
            const p_type& source;
            const std::optional<std::vector<type_alias>> argument_list;
            const bool is_mutable;
            const bool is_reference; 
        };


        struct type_primitive : type {
            type_primitive(size_t size)
                : type(symbol_type::PRIMITIVE), size(size) {}

            size_t size;
        };

        struct type_struct : type {
            type_struct()
                : type(symbol_type::STRUCT) {}
        };

        struct type_array : type {
            type_array(const size_t size, p_type& content_type)
                : type(symbol_type::ARRAY), size(size), content_type(std::move(content_type)) {}

            const size_t size;
            const p_type content_type;
        };

        struct sym_function : symbol {
            sym_function(const type_alias& return_type, const std::vector<type_alias>& parameter_type_list)
                : symbol(symbol_type::FUNCTION), return_type(std::move(return_type)), parameter_type_list(std::move(parameter_type_list)) {}

            const type_alias return_type;
            const std::vector<type_alias> parameter_type_list;
        };

        struct crt_root : crate {
            crt_root()
                : crate(symbol_type::ROOT) {}
        };

        struct crt_namespace : crate {
            crt_namespace()
                : crate(symbol_type::NAMESPACE) {}
        };
    }
}
