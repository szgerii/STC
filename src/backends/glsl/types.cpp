#include "backends/glsl/types.h"

#include <format>

namespace stc::glsl {

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
std::string type_str(const TypeDescriptor& td, const TypePool& type_pool) {
    if (td.is<VoidTD>())
        return "void";

    if (td.is<BoolTD>())
        return "bool";

    if (td.is<IntTD>()) {
        auto [width, is_signed] = td.as<IntTD>();
        assert(width == 32 && "invalid glsl int width");

        return is_signed ? "int" : "uint";
    }

    if (td.is<FloatTD>()) {
        auto [width, _enc] = td.as<FloatTD>();
        assert((width == 32 || width == 64) && "invalid glsl floating point width");

        return width == 32 ? "float" : "double";
    }

    if (td.is_vector()) {
        auto [comp_type_id, size] = td.as<VectorTD>();
        const auto& comp_td       = type_pool.get_td(comp_type_id);
        assert(comp_td.is_scalar() && "non-scalar vector component type");

        return type_prefix(comp_td) + "vec" + std::to_string(size);
    }

    if (td.is_matrix()) {
        auto [col_type_id, col_count] = td.as<MatrixTD>();
        const auto& col_td            = type_pool.get_td(col_type_id);
        assert(col_td.is_vector() && "non-vector matrix column type");

        auto [comp_type_id, row_count] = col_td.as<VectorTD>();
        const auto& comp_td            = type_pool.get_td(comp_type_id);
        assert(comp_td.is<FloatTD>() && "non-floating point matrix component type");

        return std::format("{}mat{}x{}", type_prefix(comp_td), col_count, row_count);
    }

    if (td.is_array()) {
        std::string dims_str{};
        dims_str.reserve(16);

        const TypeDescriptor* it_td = &td;
        do {
            dims_str += std::format("[{}]", it_td->as<ArrayTD>().length);
            auto [elem_type_id, dim_length] = it_td->as<ArrayTD>();

            it_td = &type_pool.get_td(elem_type_id);
        } while (it_td->is_array());

        return type_str(*it_td, type_pool) + dims_str;
    }

    if (td.is_custom()) {
        auto [data_ptr] = td.as<StructTD>();
        assert(data_ptr != nullptr && "StructTD without struct data");

        return data_ptr->name;
    }

    assert(false && "missing type case in glsl code gen's type_str");
    return "???";
}

} // namespace stc::glsl
