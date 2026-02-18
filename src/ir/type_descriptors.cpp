#include <cassert>
#include <format>

#include "ir/type_descriptors.h"
#include "ir/type_pool.h"

namespace {

using FPEnc = stc::ir::FloatTD::Encoding;

[[nodiscard]] std::string to_string(FPEnc enc) {
    switch (enc) {
        case FPEnc::ieee754:
            return "IEEE-754";

        case FPEnc::bfloat16:
            return "bfloat16";

        case FPEnc::f8e4m3:
            return "F8 E4/M3";

        case FPEnc::f8e5m2:
            return "F8 E5/M2";
    }

    throw std::logic_error{"Unaccounted floating point encoding"};
}

} // namespace

namespace stc::ir {

bool TypeDescriptor::operator==(const TypeDescriptor& other) const {
    return type_data.index() == other.type_data.index() && type_data == other.type_data;
}

[[nodiscard]] std::string to_string(const TypeDescriptor& type, const TypePool& type_pool) {
    auto visitor = [&type_pool](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, VoidTD>) {
            return "void";
        } else if constexpr (std::is_same_v<T, BoolTD>) {
            return "bool";
        } else if constexpr (std::is_same_v<T, IntTD>) {
            return std::format("{}i{}", arg.signedness ? "s" : "u", arg.width);
        } else if constexpr (std::is_same_v<T, FloatTD>) {
            return std::format("f{}{}{}", arg.width, FloatTD::required_width(arg.enc) ? "!" : "",
                               arg.enc == FloatTD::Encoding::ieee754
                                   ? ""
                                   : std::format(" ({})", ::to_string(arg.enc)));
        } else if constexpr (std::is_same_v<T, VectorTD>) {
            return std::format("vec{}<{}>", arg.comp_count,
                               to_string(type_pool.get(arg.comp_type_id), type_pool));
        } else if constexpr (std::is_same_v<T, MatrixTD>) {
            return std::format("matrix ({}x {})", arg.column_count,
                               to_string(type_pool.get(arg.column_type_id), type_pool));
        } else if constexpr (std::is_same_v<T, ArrayTD>) {
            const TypeDescriptor& el_type = type_pool.get(arg.elem_type_id);
            return std::format("array[{}] of {}", arg.length,
                               std::holds_alternative<ArrayTD>(el_type.type_data)
                                   ? std::format("({})", to_string(el_type, type_pool))
                                   : to_string(el_type, type_pool));
        } else if constexpr (std::is_same_v<T, StructTD>) {
            // TODO: list of fields
            return std::format("struct {}", arg.name);
        } else {
            static_assert(false, "missing visitor case(s)");
        }
    };

    return std::visit(visitor, type.type_data);
}

} // namespace stc::ir
