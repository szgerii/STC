#include "common/utils.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

#include "types/type_pool.h"

namespace stc::types {

const TypeDescriptor& TypePool::get_td(TypeId id) const {
    assert(!id.is_null() && "get_td called with null id");
    assert(arena.get_ptr<TypeDescriptor>(id) != nullptr && "arena returned null for type id");

    return *arena.get_ptr<TypeDescriptor>(id);
}

TypeId TypePool::get(TDVariantType type) const {
    auto it = pool.find(type);

    return it != pool.end() ? it->second : TypeId::null_id();
}

TypeId TypePool::insert_or_get(TDVariantType type, bool fail_on_get) {
    TypeId pooled_id = get(type);

    if (pooled_id != TypeId::null_id()) {
        if (fail_on_get)
            throw std::logic_error{"Type already in pool for insert_or_get call that should have "
                                   "resulted in an insertion"};

        return pooled_id;
    }

    // doesn't use arena.emplace to keep TypeDescriptor ctors private to TDs and TypePool
    auto [id, mem] = arena.allocate_for<TypeDescriptor>();
    new (mem) TypeDescriptor{type};

    pool[type] = id;
    return static_cast<TypeId>(id);
}

TypeId TypePool::bool_td() {
    return insert_or_get(BoolTD{});
}

TypeId TypePool::int_td(uint32_t width, bool is_signed) {
    return insert_or_get(IntTD{width, is_signed});
}

TypeId TypePool::float_td(uint32_t width, FloatTD::Encoding encoding) {
    std::optional<uint32_t> req_width = FloatTD::required_width(encoding);

    if (req_width && *req_width != width)
        throw std::invalid_argument{
            "Invalid floating point type width provided for the given encoding"};

    return insert_or_get(FloatTD{width, encoding});
}

TypeId TypePool::vector_td(TypeId component_type_id, uint32_t component_count) {
    const TypeDescriptor& component_type = get_td(component_type_id);

    if (!component_type.is_scalar())
        throw std::logic_error{"Cannot create vector type with non-scalar component type"};

    if (component_count < 2)
        throw std::logic_error{"Cannot create vector type with component count less than 2"};

    return insert_or_get(VectorTD{component_type_id, component_count});
}

TypeId TypePool::matrix_td(TypeId column_type_id, uint32_t column_count) {
    const TypeDescriptor& column_type = get_td(column_type_id);

    if (!column_type.is<VectorTD>())
        throw std::logic_error{"Cannot create matrix type with non-vector column type"};

    if (column_count < 2)
        throw std::logic_error{"Cannot create matrix type with column count less than 2"};

    return insert_or_get(MatrixTD{column_type_id, column_count});
}

TypeId TypePool::array_td(TypeId element_type_id, uint32_t length) {
    const TypeDescriptor& element_type = get_td(element_type_id);

    if (element_type.is_void())
        throw std::logic_error{"Cannot create array type with void as element type"};

    return insert_or_get(ArrayTD{element_type_id, length});
}

TypeId TypePool::method_td(TypeId ret_type, const std::vector<TypeId>& param_types) {
    MethodSig sig{ret_type, param_types};

    // stack alloced sig can be used for lookup
    TypeId lookup = get(MethodTD{&sig});
    if (!lookup.is_null())
        return lookup;

    if (ret_type.is_null())
        throw std::logic_error{"Cannot create method type with null as return type"};

    if (std::ranges::any_of(param_types, [](TypeId ptype) { return ptype.is_null(); }))
        throw std::logic_error{"Cannot create method type with null as a parameter's type"};

    // but when creating the interned instance, heap (arena) alloc is required
    // has separate allocs, because most invocations are expected to be returned from the pool

    MethodSig* sig_ptr             = nullptr;
    std::tie(std::ignore, sig_ptr) = arena.emplace<MethodSig>(ret_type, param_types);
    assert(sig_ptr != nullptr);

    return insert_or_get(MethodTD{sig_ptr}, true);
}

TypeId TypePool::func_td(SymbolId fn_name) {
    if (fn_name.is_null())
        throw std::logic_error{"Trying to create function type with null as the identifier symbol "
                               "(use any_func_td if this is intended)"};

    return insert_or_get(FunctionTD{fn_name});
}

// this is separate from func_td to make creation of generic function types as explicit as possible
TypeId TypePool::any_func_td() {
    // doesn't register any_func into pool, it's stored separately (but in the same arena)
    // this way a concrete function with null symbol id is treated separately from an any_func
    if (any_func.is_null()) {
        auto [id, mem] = arena.allocate_for<TypeDescriptor>();
        new (mem) TypeDescriptor{FunctionTD{SymbolId::null_id()}};

        any_func = TypeId{id};
    }

    return any_func;
}

bool TypePool::is_any_func(TypeId type) const {
    return !any_func.is_null() && any_func == type;
}

bool TypePool::is_any_func(const TypeDescriptor* fn_td) const {
    return !any_func.is_null() && &get_td(any_func) == fn_td;
}

TypeId TypePool::builtin_td(BuiltinKind kind) {
    TypeId id = get(BuiltinTD{kind});
    assert(!id.is_null() && "built-in type not found in pool");
    return id;
}

TypeId TypePool::get_struct_td(SymbolId name) {
    if (auto it = struct_map.find(name); it != struct_map.end())
        return it->second;

    return TypeId::null_id();
}

TypeId TypePool::make_struct_td(SymbolId name, std::vector<StructData::FieldInfo> fields,
                                const SymbolPool& sym_pool) {
    std::string_view struct_name = sym_pool.get_symbol(name);

    if (struct_name.empty())
        throw std::logic_error{"Cannot create struct type with an empty type name"};

    if (std::ranges::any_of(fields, [&sym_pool](const StructData::FieldInfo& f) {
            std::string_view sym = sym_pool.get_symbol(f.name);
            return sym.empty();
        }))
        throw std::logic_error{"Cannot create struct type with an empty field name"};

    if (has_duplicates(fields, [](const auto& fi) { return fi.name; }))
        throw std::logic_error{"Field names have to be unique inside structs"};

    if (struct_map.contains(name))
        throw std::logic_error{"Cannot create two structs with identical names"};

    StructData* data_ptr            = nullptr;
    std::tie(std::ignore, data_ptr) = arena.emplace<StructData>(name, std::move(fields));
    assert(data_ptr != nullptr);

    TypeId t_id = insert_or_get(StructTD{data_ptr}, true);

    struct_map[data_ptr->name] = t_id;

    return t_id;
}

void TypePool::register_builtin_str(BuiltinKind kind, std::string str) {
    bool inserted = builtin_str_map.try_emplace(kind, str).second;

    if (!inserted)
        throw std::logic_error{
            std::format("Trying to reinsert builtin type string representation for kind {}", kind)};
}

void TypePool::clear_builtin_str_map() {
    builtin_str_map.clear();
}

std::string TypePool::builtin_kind_to_str(BuiltinKind kind) const {
    auto it = builtin_str_map.find(kind);

    if (it == builtin_str_map.end())
        return "unknown builtin type";

    return it->second;
}

} // namespace stc::types