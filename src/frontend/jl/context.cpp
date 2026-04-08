#include "frontend/jl/context.h"

namespace stc::jl {

void JLCtx::init_jl_types() {
    _jl_Bool_t    = type_pool.bool_td();
    _jl_Int8_t    = type_pool.int_td(8, true);
    _jl_Int16_t   = type_pool.int_td(16, true);
    _jl_Int32_t   = type_pool.int_td(32, true);
    _jl_Int64_t   = type_pool.int_td(64, true);
    _jl_Int128_t  = type_pool.int_td(128, true);
    _jl_UInt8_t   = type_pool.int_td(8, false);
    _jl_UInt16_t  = type_pool.int_td(16, false);
    _jl_UInt32_t  = type_pool.int_td(32, false);
    _jl_UInt64_t  = type_pool.int_td(64, false);
    _jl_UInt128_t = type_pool.int_td(128, false);
    _jl_Float16_t = type_pool.float_td(16);
    _jl_Float32_t = type_pool.float_td(32);
    _jl_Float64_t = type_pool.float_td(64);
    _jl_Nothing_t = type_pool.builtin_td(BuiltinTypeKind::Nothing);
    _jl_String_t  = type_pool.builtin_td(BuiltinTypeKind::String);
    _jl_Symbol_t  = type_pool.builtin_td(BuiltinTypeKind::Symbol);

    type_pool.clear_builtin_str_map();
    type_pool.register_builtin_str(BuiltinTypeKind::Nothing, "Nothing");
    type_pool.register_builtin_str(BuiltinTypeKind::String, "String");
    type_pool.register_builtin_str(BuiltinTypeKind::Symbol, "Symbol");

    // // clang-format off
    // #define X(type) assert(_##type != types::TypeId::null_id() && #type " not initialized by
    // init_jl_types");
    //     #include "frontend/jl/node_defs/types.def"
    // #undef X
    // // clang-format on
}

} // namespace stc::jl
