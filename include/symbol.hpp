#pragma once

#include <memory>
#include <unordered_map>

namespace core {
    namespace sym {
        enum class symbol_type {
            NAMESPACE,
            FUNCTION,
            ROOT
        };

        struct symbol {
            symbol(const symbol_type type)
                : type(type) {}

            const symbol_type type;
        };

        using p_symbol = std::unique_ptr<symbol>;

        // A crate is a symbol that is guaranteed to hold other symbols.
        struct crate : symbol {
            crate(const symbol_type type)
                : symbol(type) {}

            std::unordered_map<std::string, p_symbol> symbol_list;
        };

        using p_crate = std::unique_ptr<crate>;

        struct crate_root : crate {
            crate_root()
                : crate(symbol_type::ROOT) {}
        };

        struct crate_namespace :crate {
            crate_namespace()
                : crate(symbol_type::NAMESPACE) {}
        };
    }
}
