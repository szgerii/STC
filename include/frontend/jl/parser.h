#pragma once

#include "julia_guard.h"

#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"
#include "sir/context.h"

#define PARSER_DECL(name) NodeId parse_##name(jl_expr_t* expr, size_t nargs)

namespace stc::jl {

class JLParser {
    JLCtx& ctx;
    rt::JuliaSymbolCache& sym_cache;

    SrcLocationId cur_loc;
    bool _success = true;

    using ParseCallback = NodeId (JLParser::*)(jl_value_t*);

public:
    std::optional<std::string> fallback_file = std::nullopt;

    explicit JLParser(JLCtx& ctx)
        : ctx{ctx}, sym_cache{ctx.jl_env.symbol_cache} {

        std::ignore = ctx.src_info_pool.get_file("dummy file");
        cur_loc     = ctx.src_info_pool.get_location(1, 1);
    }

    NodeId parse(jl_value_t* node);
    NodeId parse_code(std::string_view code);

    bool success() const { return _success; }

private:
    NodeId parse_expr(jl_expr_t* expr);

    NodeId parse_var_decl(jl_expr_t* expr, size_t nargs);
    NodeId parse_method_decl(jl_expr_t* expr, size_t nargs);
    NodeId parse_assignment(jl_expr_t* expr, size_t nargs);
    NodeId parse_update_assignment(jl_expr_t* expr, size_t nargs);
    NodeId parse_block(jl_expr_t* expr, size_t nargs);
    NodeId parse_call(jl_expr_t* expr, size_t nargs);
    NodeId parse_if(jl_expr_t* expr, size_t nargs);
    NodeId parse_while(jl_expr_t* expr, size_t nargs);
    NodeId parse_return(jl_expr_t* expr, size_t nargs);
    NodeId parse_dot_chain(jl_expr_t* expr, size_t nargs);
    NodeId parse_vect(jl_expr_t* expr, size_t nargs);
    NodeId parse_ref(jl_expr_t* expr, size_t nargs);
    NodeId parse_log_op(jl_expr_t* expr, size_t nargs);
    NodeId parse_struct(jl_expr_t* expr, size_t nargs);

    // helper parser functions, not participating in the regular parse_expr flow
    std::pair<jl_value_t*, TypeId> parse_type_annotation(jl_expr_t* annot);
    jl_value_t* unwrap_layout_qual(jl_expr_t* lq_expr, std::vector<QualKind>& quals,
                                   LQPayload& lq_payloads);
    NodeId parse_qualified_decl(jl_value_t* qualified_expr,
                                ParseCallback next_parser = &JLParser::parse);
    NodeId parse_param_decl(jl_value_t* param_v);
    NodeId parse_field_decl(jl_value_t* field_decl_v);
    NodeId parse_tuple_assignment(jl_expr_t* expr);

    NodeId parse_method_decl(jl_value_t* val) {
        jl_expr_t* expr = jl_is_expr(val) ? safe_cast<jl_expr_t>(val) : nullptr;

        if (expr != nullptr && (expr->head == sym_cache.function || expr->head == sym_cache.eq))
            return parse_method_decl(expr, jl_expr_nargs(expr));

        return internal_error("unexpected parse_method_decl invocation");
    }

    NodeId parse_var_decl(jl_value_t* val) {
        jl_expr_t* expr = jl_is_expr(val) ? safe_cast<jl_expr_t>(val) : nullptr;

        if (expr != nullptr && (expr->head == sym_cache.global || expr->head == sym_cache.local ||
                                expr->head == sym_cache.eq || expr->head == sym_cache.dbl_col))
            return parse_var_decl(expr, jl_expr_nargs(expr));

        return internal_error("unexpected parse_var_decl invocation");
    }

    template <typename T, typename... Args>
    NodeId emplace_node(Args&&... args) {
        return ctx.emplace_node<T>(std::forward<Args>(args)...).first;
    }

    NodeId emplace_decl_ref(SrcLocationId loc, SymbolId sym) {
        NodeId sym_lit = emplace_node<SymbolLiteral>(loc, sym);
        if (sym_lit.is_null())
            return NodeId::null_id();

        return emplace_node<DeclRefExpr>(loc, sym_lit);
    }

    NodeId emplace_decl_ref(SrcLocationId loc, std::string_view sym) {
        return emplace_decl_ref(loc, ctx.sym_pool.get_id(sym));
    }

    NodeId emplace_decl_ref(SrcLocationId loc, jl_sym_t* sym) {
        return emplace_decl_ref(loc, jl_symbol_name(sym));
    }

    TypeId resolve_type(jl_value_t* type);
    NodeId fail(std::string_view msg, SrcLocationId loc_id = SrcLocationId::null_id());
    NodeId internal_error(std::string_view msg, SrcLocationId loc_id = SrcLocationId::null_id());
};

#undef PARSER_DECL

} // namespace stc::jl
