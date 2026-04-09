#pragma once

#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"
#include "frontend/jl/rt/env.h"

namespace stc::jl {

inline std::string mod_chain_to_path(const std::vector<NodeId>& chain, const JLCtx& ctx,
                                     size_t first_n = 0) {
    static constexpr size_t MOD_NAME_LENGTH_GUESS = 5U;

    if (first_n == 0 || first_n > chain.size())
        first_n = chain.size();

    std::string result{};
    result.reserve(chain.size() * (MOD_NAME_LENGTH_GUESS + 1)); // + 1 for dots

    for (size_t i = 0; i < first_n; i++) {
        const auto* sym_lit = ctx.get_and_dyn_cast<SymbolLiteral>(chain[i]);

        if (sym_lit == nullptr)
            throw std::logic_error{"non-symbol-literal node in module lookup chain"};

        result += ctx.get_sym(sym_lit->value);

        if (i + 1 != first_n)
            result += '.';
    }

    return result;
}

} // namespace stc::jl
