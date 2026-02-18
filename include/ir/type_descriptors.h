#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

namespace stc::ir {

// basic types modelled after SPIR-V's unified specs
// some liberties have been taken for a more general IR
// this includes allowing non-float scalars as matrix component types,
// these can potentially be rewritten as arrays in the output (similarly to slang)

// ctor key (passkey) pattern from:
// https://playfulprogramming.com/posts/a-forgotten-idiom-revisited-pass-key/

using TypeId = size_t;
class TypePool;

struct TypeDescriptorCtorKey {
    friend TypePool;

private:
    TypeDescriptorCtorKey() = default;
};

struct VoidTD {
    explicit VoidTD(TypeDescriptorCtorKey) {}
    VoidTD(const VoidTD&)            = delete;
    VoidTD(VoidTD&&)                 = default;
    VoidTD& operator=(const VoidTD&) = delete;
    VoidTD& operator=(VoidTD&&)      = default;

    constexpr bool operator==(const VoidTD&) const = default;
};

struct BoolTD {
    explicit BoolTD(TypeDescriptorCtorKey) {}
    BoolTD(const BoolTD&)            = delete;
    BoolTD(BoolTD&&)                 = default;
    BoolTD& operator=(const BoolTD&) = delete;
    BoolTD& operator=(BoolTD&&)      = default;

    constexpr bool operator==(const BoolTD&) const = default;
};

struct IntTD {
    uint32_t width;
    bool signedness;

    constexpr explicit IntTD(TypeDescriptorCtorKey, uint32_t width, bool signedness)
        : width(width), signedness(signedness) {}

    IntTD(const IntTD&)            = delete;
    IntTD(IntTD&&)                 = default;
    IntTD& operator=(const IntTD&) = delete;
    IntTD& operator=(IntTD&&)      = default;

    constexpr bool operator==(const IntTD&) const = default;
};

struct FloatTD {
    enum class Encoding : uint8_t {
        ieee754,
        bfloat16,
        f8e4m3,
        f8e5m2
    };

    constexpr static std::optional<uint32_t> required_width(Encoding enc) {
        switch (enc) {
            case Encoding::ieee754:
                return std::nullopt;
            case Encoding::bfloat16:
                return 16;
            case Encoding::f8e4m3:
            case Encoding::f8e5m2:
                return 8;
        }

        throw std::logic_error{"Unaccounted float encoding in enumeration"};
    }

    uint32_t width;
    Encoding enc;

    constexpr explicit FloatTD(TypeDescriptorCtorKey, uint32_t width,
                               Encoding enc = Encoding::ieee754)
        : width(width), enc(enc) {}

    FloatTD(const FloatTD&)            = delete;
    FloatTD(FloatTD&&)                 = default;
    FloatTD& operator=(const FloatTD&) = delete;
    FloatTD& operator=(FloatTD&&)      = default;

    constexpr bool operator==(const FloatTD&) const = default;
};

struct VectorTD {
    TypeId comp_type_id;
    uint32_t comp_count;

    constexpr explicit VectorTD(TypeDescriptorCtorKey, TypeId component_type_id,
                                uint32_t component_count)
        : comp_type_id(component_type_id), comp_count(component_count) {}

    VectorTD(const VectorTD&)            = delete;
    VectorTD(VectorTD&&)                 = default;
    VectorTD& operator=(const VectorTD&) = delete;
    VectorTD& operator=(VectorTD&&)      = default;

    constexpr bool operator==(const VectorTD&) const = default;
};

struct MatrixTD {
    TypeId column_type_id;
    uint32_t column_count;

    constexpr explicit MatrixTD(TypeDescriptorCtorKey, TypeId column_type_id, uint32_t column_count)
        : column_type_id(column_type_id), column_count(column_count) {}

    MatrixTD(const MatrixTD&)            = delete;
    MatrixTD(MatrixTD&&)                 = default;
    MatrixTD& operator=(const MatrixTD&) = delete;
    MatrixTD& operator=(MatrixTD&&)      = default;

    constexpr bool operator==(const MatrixTD&) const = default;
};

struct ArrayTD {
    TypeId elem_type_id;
    uint32_t length;

    explicit ArrayTD(TypeDescriptorCtorKey, TypeId element_type_id, uint32_t length)
        : elem_type_id(element_type_id), length(length) {}

    ArrayTD(const ArrayTD&)            = delete;
    ArrayTD(ArrayTD&&)                 = default;
    ArrayTD& operator=(const ArrayTD&) = delete;
    ArrayTD& operator=(ArrayTD&&)      = default;

    bool operator==(const ArrayTD& other) const = default;
};

struct StructTD {
    struct FieldInfo {
        TypeId type;
        std::string name;

        bool operator==(const FieldInfo& other) const = default;
    };

    std::string name;
    std::vector<FieldInfo> fields;

    explicit StructTD(TypeDescriptorCtorKey, std::string name, std::vector<FieldInfo> fields)
        : name(std::move(name)), fields(std::move(fields)) {}

    StructTD(const StructTD&)            = delete;
    StructTD(StructTD&&)                 = default;
    StructTD& operator=(const StructTD&) = delete;
    StructTD& operator=(StructTD&&)      = default;

    bool operator==(const StructTD& other) const = default;
};

using TDVariantType =
    std::variant<VoidTD, BoolTD, IntTD, FloatTD, VectorTD, MatrixTD, ArrayTD, StructTD>;

template <typename T>
concept TypeDescriptorT =
    std::is_same_v<T, VoidTD> || std::is_same_v<T, BoolTD> || std::is_same_v<T, IntTD> ||
    std::is_same_v<T, FloatTD> || std::is_same_v<T, VectorTD> || std::is_same_v<T, MatrixTD> ||
    std::is_same_v<T, ArrayTD> || std::is_same_v<T, StructTD>;

struct TypeDescriptor {
    TDVariantType type_data;

    TypeDescriptor(TypeDescriptorCtorKey, TDVariantType type_data)
        : type_data{std::move(type_data)} {}

    TypeDescriptor(const TypeDescriptor&)            = delete;
    TypeDescriptor(TypeDescriptor&&)                 = default;
    TypeDescriptor& operator=(const TypeDescriptor&) = delete;
    TypeDescriptor& operator=(TypeDescriptor&&)      = default;

    template <TypeDescriptorT T>
    constexpr bool is() const {
        return std::holds_alternative<T>(type_data);
    }

    constexpr bool is_void() const { return is<VoidTD>(); }
    constexpr bool is_scalar() const { return is<BoolTD>() || is<IntTD>() || is<FloatTD>(); }
    constexpr bool is_custom() const { return is<StructTD>(); }

    template <TypeDescriptorT T>
    constexpr T as() const {
        return std::get<T>(type_data);
    }

    bool operator==(const TypeDescriptor& other) const;
};

std::string to_string(const TypeDescriptor& type, const TypePool& type_pool);

} // namespace stc::ir