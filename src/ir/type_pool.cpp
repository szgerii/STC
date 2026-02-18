#include <cassert>

#include "common/utils.h"
#include "ir/type_pool.h"

namespace stc::ir {

void TypePool::assert_valid_id(TypeId id) const {
    assert(id < pool.size() && "TypeId outside the bounds of the pool");
}

const TypeDescriptor& TypePool::get(TypeId id) const {
    assert_valid_id(id);

    return pool[id];
}

TypeId TypePool::insert(TDVariantType&& type) {
    if (purge_duplicates) {
        TypeId id = 0;
        for (auto&& td : pool) {
            if (td.type_data == type) return id;

            id++;
        }
    }

    pool.emplace_back(td_key, std::move(type));

    return pool.size() - 1;
}

TypeId TypePool::make_void_t() {
    return insert(VoidTD{td_key});
}

TypeId TypePool::make_bool_t() {
    return insert(BoolTD{td_key});
}

TypeId TypePool::make_int_t(uint32_t width, bool signedness) {
    return insert(IntTD{td_key, width, signedness});
}

TypeId TypePool::make_float_t(uint32_t width, FloatTD::Encoding encoding) {
    std::optional<uint32_t> req_width = FloatTD::required_width(encoding);

    if (req_width && *req_width != width)
        throw std::invalid_argument{
            "Invalid floating point type width provided for the given encoding"};

    return insert(FloatTD{td_key, width, encoding});
}

TypeId TypePool::make_vector_t(TypeId component_type_id, uint32_t component_count) {
    const TypeDescriptor& component_type = get(component_type_id);

    if (!component_type.is_scalar())
        throw std::logic_error{"Cannot create vector type with non-scalar component type"};

    if (component_count < 2)
        throw std::logic_error{"Cannot create vector type with component count less than 2"};

    return insert(VectorTD{td_key, component_type_id, component_count});
}

TypeId TypePool::make_matrix_t(TypeId column_type_id, uint32_t column_count) {
    const TypeDescriptor& column_type = get(column_type_id);

    if (!column_type.is<VectorTD>())
        throw std::logic_error{"Cannot create matrix type with non-vector column type"};

    if (column_count < 2)
        throw std::logic_error{"Cannot create matrix type with column count less than 2"};

    return insert(MatrixTD{td_key, column_type_id, column_count});
}

TypeId TypePool::make_array_t(TypeId element_type_id, uint32_t length) {
    const TypeDescriptor& element_type = get(element_type_id);

    if (element_type.is_void())
        throw std::logic_error{"Cannot create array type with void as element type"};

    return insert(ArrayTD{td_key, element_type_id, length});
}

TypeId TypePool::make_struct_t(std::string name, std::vector<StructTD::FieldInfo> fields) {
    bool duplicate_field_names = has_duplicates(
        fields, [](const StructTD::FieldInfo& fi) -> std::string_view { return fi.name; });

    if (duplicate_field_names)
        throw std::logic_error{"Cannot create struct type with duplicate field names"};

    // TODO: proper identifier format enforcement
    if (name.empty()) throw std::logic_error{"Cannot create struct type with an empty type name"};
    for (const auto& fi : fields) {
        if (fi.name.empty())
            throw std::logic_error{"Cannot create struct type with an empty field name"};
    }

    return insert(StructTD{td_key, std::move(name), std::move(fields)});
}

} // namespace stc::ir