#pragma once

#include <cassert>
#include <functional>
#include <string>
#include <unordered_set>
#include <vector>

#include "config.h"

namespace stc {

struct SrcFile {
    std::string_view path;
};

struct SrcLocation {
    const SrcFile* file;
    size_t line;
    size_t col;

    SrcLocation(const SrcFile* file, size_t line, size_t col)
        : file(file), line(line), col(col) {
        assert(file && line > 0 && col > 0);
    }
};

std::nullptr_t report(SrcLocation location, std::string_view msg, bool is_warning);
std::nullptr_t error(SrcLocation location, std::string_view msg);
std::nullptr_t warning(SrcLocation location, std::string_view msg);

inline std::string indent(size_t level) {
    return STC_USE_TABS ? std::string(level, '\t') : std::string(level * STC_DUMP_INDENT, ' ');
}

template <typename T, typename Projection = std::identity>
requires std::regular_invocable<Projection&, const T&> && requires {
    // unordered set is instantiable with the return type of Projection
    std::unordered_set<std::remove_cvref_t<std::invoke_result_t<Projection&, const T&>>>{};
}
bool has_duplicates(const std::vector<T>& vec, Projection proj = {}) {
    using ProjType = std::remove_cvref_t<std::invoke_result_t<Projection&, const T&>>;

    std::unordered_set<ProjType> buffer;
    buffer.reserve(vec.size());

    for (const T& el : vec) {
        if (!buffer.insert(std::invoke(proj, el)).second) return true;
    }

    return false;
}

} // namespace stc