#include "ast/symbol_pool.h"
#include "types/type_descriptors.h"
#include "types/type_pool.h"
#include <catch2/catch_test_macros.hpp>

using namespace stc;
using namespace stc::types;

// if TypePool tests pass, that also verifies that TypeDescriptor is working properly

TEST_CASE("TypePool init", "[TypePool]") {
    TypePool pool{1};

    SECTION("void auto initialized to 1") {
        TypeId void_id = pool.void_td();

        REQUIRE_FALSE(void_id.is_null());
        REQUIRE(void_id.value == 1U);

        const TypeDescriptor& td = pool.get_td(void_id);
        REQUIRE(td.is_void());
        REQUIRE(pool.is_type_of<VoidTD>(void_id));
    }
}

TEST_CASE("scalar interning", "[TypePool]") {
    TypePool pool{1};

    SECTION("same type id") {
        REQUIRE(pool.bool_td() == pool.bool_td());

        TypeId int32_a = pool.int_td(32, true);
        TypeId int32_b = pool.int_td(32, true);

        REQUIRE(int32_a == int32_b);

        TypeId uint32_a = pool.int_td(32, false);
        TypeId uint32_b = pool.int_td(32, false);

        REQUIRE(uint32_a == uint32_b);

        TypeId float32_a = pool.float_td(32);
        TypeId float32_b = pool.float_td(32);

        REQUIRE(float32_a == float32_b);
    }

    SECTION("different type id") {
        TypeId int32_signed   = pool.int_td(32, true);
        TypeId int32_unsigned = pool.int_td(32, false);
        TypeId int16_signed   = pool.int_td(16, true);

        REQUIRE(int32_signed != int32_unsigned);
        REQUIRE(int32_signed != int16_signed);
    }
}

TEST_CASE("vec/mat", "[TypePool]") {
    TypePool pool{1};
    TypeId f32 = pool.float_td(32);
    TypeId i32 = pool.int_td(32, true);

    SECTION("vector interning") {
        TypeId vec3_f32_a = pool.vector_td(f32, 3);
        TypeId vec3_f32_b = pool.vector_td(f32, 3);
        TypeId vec4_f32   = pool.vector_td(f32, 4);
        TypeId vec3_i32   = pool.vector_td(i32, 3);

        REQUIRE(vec3_f32_a == vec3_f32_b);
        REQUIRE(vec3_f32_a != vec4_f32);
        REQUIRE(vec3_f32_a != vec3_i32);

        const TypeDescriptor& v3 = pool.get_td(vec3_f32_a);
        REQUIRE(v3.is_vector());
        REQUIRE(v3.as<VectorTD>().component_count == 3);
    }

    SECTION("mat interning") {
        TypeId vec3  = pool.vector_td(f32, 3);
        TypeId ivec4 = pool.vector_td(i32, 4);

        TypeId mat3x2_a = pool.matrix_td(vec3, 2);
        TypeId mat3x2_b = pool.matrix_td(vec3, 2);
        TypeId imat4_a  = pool.matrix_td(ivec4, 4);
        TypeId imat4_b  = pool.matrix_td(ivec4, 4);

        REQUIRE(mat3x2_a == mat3x2_b);
        REQUIRE(imat4_a == imat4_b);
        REQUIRE(mat3x2_a != imat4_a);

        const TypeDescriptor& mat_td = pool.get_td(mat3x2_a);
        REQUIRE(mat_td.is_matrix());
        REQUIRE(mat_td.as<MatrixTD>().column_count == 2);
        REQUIRE(mat_td.as<MatrixTD>().column_type_id == vec3);
    }
}

TEST_CASE("structs", "[TypePool]") {
    TypePool pool{1};
    SymbolPool sym_pool{1};

    SymbolId struct_name = sym_pool.get_id("SomeStruct");
    SymbolId field_a     = sym_pool.get_id("a");
    SymbolId field_b     = sym_pool.get_id("b");

    TypeId f32  = pool.float_td(32);
    TypeId vec3 = pool.vector_td(f32, 3);
    TypeId vec4 = pool.vector_td(f32, 4);

    SECTION("creation and retrieval") {
        std::vector<StructData::FieldInfo> fields = {{field_a, vec3}, {field_b, vec4}};

        TypeId struct_id = pool.make_struct_td(struct_name, fields, sym_pool);
        REQUIRE_FALSE(struct_id.is_null());

        TypeId retrieved_id = pool.get_struct_td(struct_name);
        REQUIRE(struct_id == retrieved_id);

        const TypeDescriptor& td = pool.get_td(struct_id);
        REQUIRE(td.is_struct());

        const StructData* data = td.as<StructTD>().data;
        REQUIRE(data != nullptr);
        REQUIRE(data->name == struct_name);
        REQUIRE(data->fields.size() == 2);
        REQUIRE(data->fields[0].name == field_a);
        REQUIRE(data->fields[0].type == vec3);
    }

    SECTION("not found struct null return") {
        SymbolId wrong_name = sym_pool.get_id("some symbol");
        REQUIRE(pool.get_struct_td(wrong_name) == TypeId::null_id());
    }
}

TEST_CASE("type desc casting", "[TypeDescriptor]") {
    TypePool pool{1}; // td-s only initable by type pool
    TypeId b_id = pool.bool_td();

    const TypeDescriptor& td = pool.get_td(b_id);

    SECTION("correct cast") {
        REQUIRE(td.is_scalar());
        REQUIRE(td.is<BoolTD>());
        std::ignore = td.as<BoolTD>();
    }
}
