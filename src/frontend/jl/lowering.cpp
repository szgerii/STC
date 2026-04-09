#include "frontend/jl/lowering.h"
#include "sir/ast.h"
#include "types/type_to_string.h"

namespace {

using SIRNodeId = stc::sir::NodeId;
using namespace stc::jl;

[[nodiscard]] STC_FORCE_INLINE bool is_ret_type_allowed(const TypeDescriptor& ret_type) {
    return ret_type.is_scalar() || ret_type.is_vector() || ret_type.is_matrix() ||
           ret_type.is_array() || ret_type.is_struct() ||
           ret_type.is_builtin(BuiltinTypeKind::Nothing);
}

} // namespace

namespace stc::jl {

bool JLLoweringVisitor::pre_visit_ptr(Expr* expr) {
    swap_lower_type(expr->type);
    return true;
}

SIRNodeId JLLoweringVisitor::visit_default_case() {
    internal_error("nullptr found in the Julia AST during lowering to SIR");
    this->success = false;

    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::fail(std::string_view msg) {
    stc::error(msg);
    success = false;
    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::internal_error(std::string_view msg) {
    stc::internal_error(msg);
    success = false;
    return SIRNodeId::null_id();
}

SIRNodeId JLLoweringVisitor::visit_and_check(NodeId id) {
    SIRNodeId result = visit(id);

    if (result.is_null()) {
        if (success)
            internal_error("null_id returned by a node in the Julia -> SIR lowering visitor.");

        return SIRNodeId::null_id();
    }

    return result;
}

SIRNodeId JLLoweringVisitor::visit_ptr(Expr* node) {
    return this->dispatch_wrapper(node);
}

SIRNodeId JLLoweringVisitor::lower(NodeId global_cmpd_id) {
    auto* global_cmpd = ctx.get_and_dyn_cast<CompoundExpr>(global_cmpd_id);
    if (global_cmpd == nullptr)
        return internal_error(
            "Null id passed to Julia -> SIR lowering pass as the global scope body");

    auto& body = global_cmpd->body;

    // lift global var decls (including implicit ones) to the top
    std::vector<NodeId> prepended_exprs{};
    bool encountered_real_body = false;
    for (size_t i = 0; i < body.size();) {
        NodeId expr = body[i];

        auto* vdecl = ctx.get_and_dyn_cast<VarDecl>(expr);
        if (vdecl) {
            if (mst_to_st(vdecl->scope()) == ScopeType::Global) {
                prepended_exprs.emplace_back(expr);

                if (vdecl->initializer.is_null() || !encountered_real_body) {
                    body.erase(body.begin() + i);
                    continue;
                }

                NodeId dre = ctx.emplace_node<DeclRefExpr>(vdecl->location, expr).first;
                NodeId init_assign =
                    ctx.emplace_node<Assignment>(vdecl->location, dre, vdecl->initializer).first;
                body[i] = init_assign;

                vdecl->initializer = NodeId::null_id();

                i++;
            }

            continue;
        }

        auto* assign = ctx.get_and_dyn_cast<Assignment>(expr);
        if (assign && assign->is_implicit_decl()) {
            const auto* dre = ctx.get_and_dyn_cast<DeclRefExpr>(assign->target);
            assert(dre != nullptr);

            auto* vdecl = ctx.get_and_dyn_cast<VarDecl>(dre->decl);
            if (mst_to_st(vdecl->scope()) == ScopeType::Global) {
                body[i] = dre->decl;
                continue; // parse current line again as a global var decl, with initializer
            }

            if (!ctx.isa<MethodDecl>(dre->decl))
                encountered_real_body = true;

            i++;
            continue;
        }

        if (!ctx.isa<MethodDecl>(expr))
            encountered_real_body = true;

        i++;
    }

    body.insert(body.begin(), prepended_exprs.begin(), prepended_exprs.end());

    // lift global method decls to underneath global var decls
    std::stable_partition(body.begin() + prepended_exprs.size(), body.end(),
                          [this](NodeId expr) -> bool { return ctx.isa<MethodDecl>(expr); });

    // wrap rest of body in a main function
    size_t body_first_idx = prepended_exprs.size();
    while (body_first_idx < body.size() && ctx.isa<MethodDecl>(body[body_first_idx]))
        body_first_idx++;

    // if (body_first_idx >= body.size())
    //     return visit(global_cmpd);

    std::vector<NodeId> main_body{};

    if (body_first_idx < body.size()) {
        main_body.reserve(body.size() - body_first_idx);
        main_body.insert(main_body.end(), body.begin() + body_first_idx, body.end());
    }

    body.resize(body_first_idx); // leave one for the main method's decl

    SrcLocationId main_loc = !main_body.empty()
                                 ? ctx.get_node(main_body.back())->location
                                 : (!body.empty() ? ctx.get_node(body.back())->location
                                                  : ctx.src_info_pool.get_location(1, 1));

    NodeId main_cmpd = ctx.emplace_node<CompoundExpr>(main_loc, std::move(main_body)).first;

    NodeId main_method =
        ctx.emplace_node<MethodDecl>(main_loc, sir_ctx.sym_pool.get_id("main"), ctx.jl_Nothing_t(),
                                     std::vector<NodeId>{}, main_cmpd)
            .first;

    body.emplace_back(main_method);

    return visit(global_cmpd);
}

SIRNodeId JLLoweringVisitor::visit_VarDecl(VarDecl& var) {
    swap_lower_type(var.annot_type);

    SIRNodeId init =
        !var.initializer.is_null() ? visit_and_check(var.initializer) : SIRNodeId::null_id();

    return emplace_decl<sir::VarDecl>(&var, var.location, var.identifier, var.type, init);
}

SIRNodeId JLLoweringVisitor::visit_MethodDecl(MethodDecl& method) {
    if (method.ret_type.is_null() ||
        !is_ret_type_allowed(sir_ctx.type_pool.get_td(method.ret_type))) {

        return fail(std::format("cannot lower function with return type '{}'",
                                type_to_string(method.ret_type, sir_ctx, sir_ctx)));
    }

    std::vector<SIRNodeId> params;
    params.reserve(method.param_decls.size());

    for (NodeId param : method.param_decls)
        params.push_back(visit_and_check(param));

    SIRNodeId body = visit_and_check(method.body);

    SIRNodeId scoped_body = emplace_node<sir::ScopedStmt>(sir_ctx.get_node(body)->location, body);

    swap_lower_type(method.ret_type);

    return emplace_decl<sir::FunctionDecl>(&method, method.location, method.identifier,
                                           method.ret_type, std::move(params), scoped_body);
}

SIRNodeId JLLoweringVisitor::visit_FunctionDecl(FunctionDecl& fn) {
    std::vector<SIRNodeId> methods;
    methods.reserve(fn.methods.size());

    for (NodeId method : fn.methods)
        methods.push_back(visit_and_check(method));

    return emplace_decl<sir::CompoundStmt>(&fn, fn.location, std::move(methods));
}

SIRNodeId JLLoweringVisitor::visit_ParamDecl(ParamDecl& param) {
    if (!param.default_initializer.is_null())
        return fail("default initialized parameters are currently not supported");

    swap_lower_type(param.annot_type);

    return emplace_decl<sir::ParamDecl>(&param, param.location, param.identifier, param.type);
}

SIRNodeId JLLoweringVisitor::visit_OpaqueFunction(OpaqueFunction& opaq_fn) {
    return fail(std::format("cannot transpile unknown Julia function '{}'",
                            sir_ctx.get_sym(opaq_fn.fn_name())));
}

SIRNodeId JLLoweringVisitor::visit_StructDecl(StructDecl& struct_) {
    std::vector<SIRNodeId> fields;
    fields.reserve(struct_.field_decls.size());

    for (NodeId field : struct_.field_decls)
        fields.push_back(visit_and_check(field));

    return emplace_decl<sir::StructDecl>(&struct_, struct_.location, struct_.identifier,
                                         std::move(fields));
}

SIRNodeId JLLoweringVisitor::visit_FieldDecl(FieldDecl& field) {
    return emplace_decl<sir::FieldDecl>(&field, field.location, field.identifier, field.type);
}

SIRNodeId JLLoweringVisitor::visit_CompoundExpr(CompoundExpr& cmpd) {
    std::vector<SIRNodeId> sir_nodes;
    sir_nodes.reserve(cmpd.body.size());

    for (NodeId expr : cmpd.body)
        sir_nodes.push_back(visit_and_check(expr));

    return emplace_node<sir::CompoundStmt>(cmpd.location, std::move(sir_nodes));
}

SIRNodeId JLLoweringVisitor::visit_BoolLiteral(BoolLiteral& bool_lit) {
    SIRNodeId id = emplace_node<sir::BoolLiteral>(bool_lit.location, bool_lit.value());
    auto* bl     = sir_ctx.get_and_dyn_cast<sir::BoolLiteral>(id);
    bl->set_type(sir_ctx.type_pool.bool_td());

    return id;
}

#define GEN_INT_LITERAL_VISITOR(type, width, is_signed)                                            \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                               \
    SIRNodeId JLLoweringVisitor::visit_##type(type& lit) {                                         \
        return emplace_node<sir::IntLiteral>(lit.location,                                         \
                                             sir_ctx.type_pool.int_td((width), (is_signed)),       \
                                             std::to_string(lit.value));                           \
    }

GEN_INT_LITERAL_VISITOR(Int32Literal, 32, true)
GEN_INT_LITERAL_VISITOR(Int64Literal, 64, true)
GEN_INT_LITERAL_VISITOR(UInt8Literal, 8, false)
GEN_INT_LITERAL_VISITOR(UInt16Literal, 16, false)
GEN_INT_LITERAL_VISITOR(UInt32Literal, 32, false)
GEN_INT_LITERAL_VISITOR(UInt64Literal, 64, false)

SIRNodeId JLLoweringVisitor::visit_UInt128Literal([[maybe_unused]] UInt128Literal& lit) {
    return internal_error("unsupported UInt128 literal node not caught by sema");
}

SIRNodeId JLLoweringVisitor::visit_Float32Literal(Float32Literal& lit) {
    return emplace_node<sir::FloatLiteral>(lit.location, sir_ctx.type_pool.float_td(32),
                                           std::to_string(lit.value));
}

SIRNodeId JLLoweringVisitor::visit_Float64Literal(Float64Literal& lit) {
    return emplace_node<sir::FloatLiteral>(lit.location, sir_ctx.type_pool.float_td(64),
                                           std::to_string(lit.value));
}

SIRNodeId JLLoweringVisitor::visit_StringLiteral([[maybe_unused]] StringLiteral& lit) {
    return internal_error("unsupported String literal node not caught by sema");
}

SIRNodeId JLLoweringVisitor::visit_SymbolLiteral([[maybe_unused]] SymbolLiteral& lit) {
    return internal_error("symbol literal in AST should have been resolved by sema");
}

SIRNodeId JLLoweringVisitor::visit_ModuleLookup([[maybe_unused]] ModuleLookup& ml) {
    return internal_error("module lookup in AST should have been resolved by sema");
}

SIRNodeId JLLoweringVisitor::visit_NothingLiteral([[maybe_unused]] NothingLiteral& lit) {
    throw std::logic_error{"using nothing literal outside of a return stmt"};
}

SIRNodeId JLLoweringVisitor::visit_OpaqueNode([[maybe_unused]] OpaqueNode& opaq) {
    return internal_error("OpaqueNode node not caught by sema");
}

SIRNodeId JLLoweringVisitor::visit_GlobalRef([[maybe_unused]] GlobalRef& gref) {
    throw std::logic_error{"using unimplemented feature: global refs"};
}

SIRNodeId JLLoweringVisitor::visit_DeclRefExpr(DeclRefExpr& dre) {
    assert(ctx.isa<Decl>(dre.decl));

    Decl* decl        = ctx.get_and_dyn_cast<Decl>(dre.decl);
    SIRNodeId decl_id = SIRNodeId::null_id();

    auto it = decl_map.find(decl);
    if (it != decl_map.end())
        decl_id = it->second;
    else
        decl_id = visit_and_check(dre.decl);

    assert(!decl_id.is_null());

    return emplace_node<sir::DeclRefExpr>(dre.location, decl_id);
}

SIRNodeId JLLoweringVisitor::visit_Assignment(Assignment& assign) {
    if (assign.is_implicit_decl()) {
        auto* dre = ctx.get_and_dyn_cast<DeclRefExpr>(assign.target);

        assert(dre != nullptr && "assignment marked as an implicit declaration, but lhs is not a "
                                 "declaration reference expression");

        if (!ctx.isa<VarDecl>(dre->decl))
            throw std::logic_error{
                "implicitly declaring assignment initializes a non-variable-declaration node"};

        // should only ever return a VarDecl (or maybe a MethodDecl later on)
        SIRNodeId result = visit_and_check(dre->decl);
        assert(sir_ctx.isa<sir::VarDecl>(result));

        return result;
    }

    return emplace_node<sir::Assignment>(assign.location, visit_and_check(assign.target),
                                         visit_and_check(assign.value));
}

SIRNodeId JLLoweringVisitor::visit_FunctionCall(FunctionCall& fn_call) {
    auto* dre = ctx.get_and_dyn_cast<DeclRefExpr>(fn_call.target_fn);

    if (dre == nullptr)
        return internal_error(
            "non-declaration-reference node in FunctionCall's target_fn not caught by sema");

    // TODO: check for and verify builtins

    SymbolId fn_identifier = SymbolId::null_id();

    if (auto* decl = ctx.get_and_dyn_cast<FunctionDecl>(dre->decl))
        fn_identifier = decl->identifier;
    else if (auto* opaq = ctx.get_and_dyn_cast<OpaqueFunction>(dre->decl))
        fn_identifier = opaq->fn_name();
    else if (ctx.isa<SymbolLiteral>(dre->decl))
        return internal_error("unresolved declaration reference expression found post-sema");
    else
        return internal_error(
            "unexpected declaration kind referenced in target of a function call");

    if (fn_identifier.is_null())
        return internal_error("couldn't determine identifier for the target of a function call");

    auto make_binop = [this, &fn_call](sir::BinaryOp::OpKind kind) -> SIRNodeId {
        return emplace_node<sir::BinaryOp>(fn_call.location, kind, visit_and_check(fn_call.args[0]),
                                           visit_and_check(fn_call.args[1]));
    };

    if (fn_call.args.size() == 2) {
        if (fn_identifier == sym_plus)
            return make_binop(sir::BinaryOp::OpKind::add);
        if (fn_identifier == sym_minus)
            return make_binop(sir::BinaryOp::OpKind::sub);
        if (fn_identifier == sym_asterisk)
            return make_binop(sir::BinaryOp::OpKind::mul);
    }

    std::vector<SIRNodeId> args{};
    args.reserve(fn_call.args.size());

    for (NodeId arg : fn_call.args)
        args.push_back(visit_and_check(arg));

    return emplace_node<sir::FunctionCall>(fn_call.location, fn_identifier, std::move(args));
}

SIRNodeId JLLoweringVisitor::visit_IfExpr(IfExpr& if_) {
    SIRNodeId lo_cond = visit_and_check(if_.condition);

    SIRNodeId lo_true =
        emplace_node<sir::ScopedStmt>(if_.location, visit_and_check(if_.true_branch));

    SIRNodeId lo_false = SIRNodeId::null_id();
    if (!if_.false_branch.is_null())
        lo_false = emplace_node<sir::ScopedStmt>(if_.location, visit_and_check(if_.false_branch));

    return emplace_node<sir::IfStmt>(if_.location, lo_cond, lo_true, lo_false);
}

SIRNodeId JLLoweringVisitor::visit_WhileExpr(WhileExpr& while_) {
    SIRNodeId lo_cond = visit_and_check(while_.condition);

    SIRNodeId lo_body =
        emplace_node<sir::ScopedStmt>(while_.location, visit_and_check(while_.body));

    return emplace_node<sir::WhileStmt>(while_.location, lo_cond, lo_body);
}

SIRNodeId JLLoweringVisitor::visit_ReturnStmt(ReturnStmt& return_stmt) {
    if (return_stmt.inner.is_null() || ctx.isa<NothingLiteral>(return_stmt.inner))
        return emplace_node<sir::ReturnStmt>(return_stmt.location);

    SIRNodeId lo_inner = visit_and_check(return_stmt.inner);

    return emplace_node<sir::ReturnStmt>(return_stmt.location, lo_inner);
}

SIRNodeId JLLoweringVisitor::visit_ContinueStmt(ContinueStmt& cont_stmt) {
    return emplace_node<sir::ContinueStmt>(cont_stmt.location);
}

SIRNodeId JLLoweringVisitor::visit_BreakStmt(BreakStmt& break_stmt) {
    return emplace_node<sir::BreakStmt>(break_stmt.location);
}

} // namespace stc::jl
