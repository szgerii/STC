#pragma once

#include "ir/type_descriptors.h"
#include "ir/type_pool.h"

namespace stc::glsl {

using namespace ir;

inline std::string type_prefix(const TypeDescriptor& td) {
    assert(td.is_scalar() && "trying to get prefix of non-scalar type");

    if (td.is<BoolTD>())
        return "b";

    if (td.is<IntTD>())
        return td.as<IntTD>().is_signed ? "i" : "u";

    if (td.is<FloatTD>())
        return td.as<FloatTD>().width == 32 ? "f" : "d";

    return "???";
}

std::string type_str(const TypeDescriptor& td, const TypePool& pool);

inline std::string type_str(TypeId type_id, const TypePool& pool) {
    return type_str(pool.get_td(type_id), pool);
}

} // namespace stc::glsl
