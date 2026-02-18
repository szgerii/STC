#pragma once

#include "ir/type_descriptors.h"
#include <optional>
#include <vector>

namespace stc::ir {

// WARNING:
// make_* calls (may) invalidate previous references of TypeDescriptors returned by get(TypeId), if
// the insertion of the new type required the underlying vector to be resized.
// TODO: use deque? smart wrapper refs/ptrs?
// TODO: map for lookup (needs hashable TypeDescriptor)
class TypePool final {
public:
    explicit TypePool(size_t initial_capacity, bool purge_duplicates = true)
        : pool{}, purge_duplicates{purge_duplicates} {

        pool.reserve(initial_capacity);
    }

    const TypeDescriptor& get(TypeId id) const;

    template <TypeDescriptorT T>
    std::optional<const T&> get_if_t(TypeId t_id) const {
        assert_valid_id(t_id);
        const TypeDescriptor& td = pool[t_id];

        if (td.is<T>()) {
            return std::get<T>(td.type_data);
        }

        return std::nullopt;
    }

    template <TypeDescriptorT T>
    bool is_type_of(TypeId id) const {
        assert_valid_id(id);
        return pool[id].is<T>();
    }

    TypeId make_void_t();
    TypeId make_bool_t();
    TypeId make_int_t(uint32_t width, bool signedness);
    TypeId make_float_t(uint32_t width, FloatTD::Encoding encoding);
    TypeId make_vector_t(TypeId component_type_id, uint32_t component_count);
    TypeId make_matrix_t(TypeId column_type_id, uint32_t column_count);
    TypeId make_array_t(TypeId element_type_id, uint32_t length);
    TypeId make_struct_t(std::string name, std::vector<StructTD::FieldInfo> fields);

private:
    constexpr static TypeDescriptorCtorKey td_key{};

    TypeId insert(TDVariantType&& type);

    void assert_valid_id(TypeId id) const;

    std::vector<TypeDescriptor> pool;
    bool purge_duplicates;
};

} // namespace stc::ir