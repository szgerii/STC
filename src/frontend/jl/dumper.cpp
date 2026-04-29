#include "frontend/jl/dumper.h"
#include "frontend/jl/ast_utils.h"
#include "frontend/jl/utils.h"
#include "types/type_to_string.h"

namespace {

using namespace stc;
using namespace stc::jl;

std::string_view mst_str(MaybeScopeType mst) {
    if (mst == MaybeScopeType::Unspec)
        return "unspecified";

    return scope_str(mst_to_st(mst));
}

std::string_view decl_type_str(Decl* decl) {
    // clang-format off
    #define X(type, kind)                                                                              \
        if (isa<type>(decl)) {                                                                         \
            return #type;                                                                              \
        }

        #include "frontend/jl/node_defs/decl.def"
    #undef X
    // clang-format on

    throw std::logic_error{"Unaccounted declaration type in decl_type_str"};
}

} // namespace

namespace stc::jl {

std::string JLDumper::type_str(TypeId type_id) const {
    return type_to_string(type_id, ctx.type_pool, ctx.sym_pool);
}

std::string_view JLDumper::sym(SymbolId sym_id) const {
    auto sv = ctx.sym_pool.get_symbol_maybe(sym_id);

    if (!sv.has_value())
        return "<unknown symbol>";

    return *sv;
}

std::string JLDumper::indent() const {
    return stc::indent(indent_level, ctx.config.dump_indent, ctx.config.use_tabs);
}

void JLDumper::inc_indent() {
    indent_level += ctx.config.dump_indent;
}

void JLDumper::dec_indent() {
    indent_level -= ctx.config.dump_indent;
}

void JLDumper::dump_with_label(std::string_view label, NodeId node) {
    out << indent() << dump_label(label);
    inc_indent();
    visit(node);
    dec_indent();
}

void JLDumper::dump_with_label(std::string_view label, const std::vector<NodeId>& nodes) {
    out << indent() << dump_label(label);
    inc_indent();
    for (NodeId node : nodes)
        visit(node);
    dec_indent();
}

// CLEANUP: separate core logic of pre_visits into helper

bool JLDumper::pre_visit_id(NodeId node_id) {
    Expr* node = ctx.get_node(node_id);

    out << indent() << '[' << fmt::format("{:p}|{}|", static_cast<void*>(node), node_id.id_value())
        << (node != nullptr ? std::to_string(static_cast<uint8_t>(node->kind())) : "?") << "]\n";

    if (node != nullptr)
        out << indent() << '<' << type_str(node->type) << '|' << std::to_string(node->type)
            << ">\n";
    else
        out << indent() << "<?|?>\n";

    return true;
}

bool JLDumper::pre_visit_ptr(Expr* expr) {
    out << indent() << '[' << fmt::format("{:p}", static_cast<void*>(expr)) << "|_|"
        << (expr != nullptr ? std::to_string(static_cast<uint8_t>(expr->kind())) : "?") << "]\n";

    if (expr != nullptr)
        out << indent() << '<' << type_str(expr->type) << '|' << std::to_string(expr->type)
            << ">\n";
    else
        out << indent() << "<?|?>\n";

    return true;
}

void JLDumper::visit_VarDecl(VarDecl& var) {
    out << indent() << "VarDecl (" << mst_str(var.scope()) << "): " << sym(var.identifier) << " ("
        << (!var.annot_type.is_null() ? type_str(var.annot_type) : "unannotated") << ")\n";

    out << indent() << "is builtin: " << (var.is_builtin() ? "yes\n" : "no\n");
    out << indent() << "is silent decl: " << (var.is_silent_decl() ? "yes\n" : "no\n");

    out << indent() << dump_label("initializer");
    inc_indent();
    if (!var.initializer.is_null())
        visit(var.initializer);
    else
        out << indent() << "-\n";
    dec_indent();
}

void JLDumper::visit_MethodDecl(MethodDecl& mdecl) {
    out << indent() << "MethodDecl: " << sym(mdecl.identifier) << " -> " << type_str(mdecl.ret_type)
        << '\n';

    dump_with_label("args", mdecl.param_decls);
    dump_with_label("body", mdecl.body);
}

void JLDumper::visit_FunctionDecl(FunctionDecl& fn) {
    out << indent() << "FunctionDecl: " << sym(fn.identifier) << '\n';

    dump_with_label("methods", fn.methods);
}

void JLDumper::visit_ParamDecl(ParamDecl& param) {
    out << indent() << "ParamDecl: " << sym(param.identifier) << " (" << type_str(param.annot_type)
        << ")\n";

    if (!param.default_initializer.is_null())
        dump_with_label("default_initializer", param.default_initializer);
}

void JLDumper::visit_OpaqueFunction(OpaqueFunction& fn) {
    out << indent() << "OpaqueFunction" << (fn.is_ctor() ? " (ctor)" : "") << ": "
        << sym(fn.fn_name()) << '@' << fn.jl_function << '\n';
}

void JLDumper::visit_BuiltinFunction(BuiltinFunction& fn) {
    out << indent() << "BuiltinFunction: " << sym(fn.fn_name()) << '\n';
}

void JLDumper::visit_StructDecl(StructDecl& struct_) {
    out << indent() << "StructDecl (" << sym(struct_.identifier) << ", "
        << (struct_.is_mutable() ? "mutable" : "immutable") << "):\n";

    dump_with_label("fields", struct_.field_decls);
}

void JLDumper::visit_InterfaceBlockDecl(InterfaceBlockDecl& iface_blk) {
    out << indent() << "InterfaceBlockDecl:\n";

    out << indent() << "storage qualifier: " << iface_storage_str(iface_blk.storage_type()) << '\n';
    out << indent() << "block name: " << iface_blk.block_name() << '\n';

    out << indent() << "instance name: ";
    if (!iface_blk.instance_name().is_null())
        out << ctx.get_sym(iface_blk.instance_name()) << '\n';
    else
        out << "-\n";

    dump_with_label("fields", iface_blk.field_decls);
}

void JLDumper::visit_FieldDecl(FieldDecl& field) {
    out << indent() << "FieldDecl: " << sym(field.identifier) << " (" << type_str(field.type)
        << ")\n";
}

void JLDumper::visit_CompoundExpr(CompoundExpr& cmpd) {
    out << indent() << "CompoundExpr:\n";

    dump_with_label("body", cmpd.body);
}

void JLDumper::visit_BoolLiteral(BoolLiteral& bool_lit) {
    out << indent() << "BoolLiteral: " << (bool_lit.value() ? "true" : "false") << '\n';
}

#define GEN_LITERAL_VISITOR(type, suffix)                                                          \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                               \
    void JLDumper::visit_##type(type& lit) {                                                       \
        out << indent() << #type << ": " << lit.value << #suffix << '\n';                          \
    }

#define GEN_UINT_LITERAL_VISITOR(type, width)                                                      \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */                                               \
    void JLDumper::visit_##type(type& lit) {                                                       \
        out << indent() << #type << ": " << fmt::format("{:#0" #width "x}", lit.value) << '\n';    \
    }

GEN_LITERAL_VISITOR(Int32Literal, )
GEN_LITERAL_VISITOR(Int64Literal, )
GEN_LITERAL_VISITOR(Float32Literal, f0)
GEN_LITERAL_VISITOR(Float64Literal, )
GEN_UINT_LITERAL_VISITOR(UInt8Literal, 4)
GEN_UINT_LITERAL_VISITOR(UInt16Literal, 6)
GEN_UINT_LITERAL_VISITOR(UInt32Literal, 10)
GEN_UINT_LITERAL_VISITOR(UInt64Literal, 18)

void JLDumper::visit_UInt128Literal(UInt128Literal& lit) {
    out << indent() << "UInt128Literal: " << fmt::format("{:#034x}", lit.hi)
        << fmt::format("{:034x}", lit.lo) << '\n';
}

void JLDumper::visit_ArrayLiteral(ArrayLiteral& arr_lit) {
    out << indent() << "ArrayLiteral (size: " << std::to_string(arr_lit.members.size()) << "):\n";

    out << indent() << dump_label("members");
    inc_indent();
    for (size_t i = 0; i < arr_lit.members.size(); i++) {
        out << indent() << '[' << std::to_string(i) << "]:\n";
        visit(arr_lit.members[i]);
    }
    dec_indent();
}

void JLDumper::visit_IndexerExpr(IndexerExpr& idx_expr) {
    out << indent() << "IndexerExpr:\n";

    dump_with_label("target", idx_expr.target);

    out << indent() << dump_label("indexers");
    inc_indent();
    for (size_t i = 0; i < idx_expr.indexers.size(); i++) {
        out << indent() << '[' << std::to_string(i) << "]:\n";
        visit(idx_expr.indexers[i]);
    }
    dec_indent();
}

// CLEANUP: prettier printing for long/multiline literals with wrapping
void JLDumper::visit_StringLiteral(StringLiteral& lit) {
    out << indent() << "StringLiteral:\n";

    inc_indent();
    out << indent() << '"' << lit.value << "\"\n";
    dec_indent();
}

void JLDumper::visit_SymbolLiteral(SymbolLiteral& lit) {
    out << indent() << "SymbolLiteral: :(" << sym(lit.value) << ")\n";
}

void JLDumper::visit_FieldAccess(FieldAccess& acc) {
    out << indent() << "FieldAccess:\n";

    dump_with_label("target", acc.target);
    dump_with_label("field", acc.field_decl);
}

void JLDumper::visit_DotChain(DotChain& dc) {
    out << indent() << "DotChain:\n";

    dump_with_label("chain", dc.chain);

    out << indent() << dump_label("resolved");
    inc_indent();
    if (!dc.resolved_expr.is_null())
        visit(dc.resolved_expr);
    else
        out << indent() << "-\n";
    dec_indent();
}

void JLDumper::visit_NothingLiteral([[maybe_unused]] NothingLiteral& lit) {
    out << indent() << "nothing\n";
}

void JLDumper::visit_OpaqueNode(OpaqueNode& opaq) {
    out << indent() << "OpaqueNode: " << opaq.jl_value << " (" << sym(opaq.jl_type_name) << ")\n";
}

void JLDumper::visit_GlobalRef(GlobalRef& ref) {
    out << indent() << "GlobalRef: " << sym(ref.sym_name) << " in module at " << ref.module;
}

void JLDumper::visit_ImplicitCast(ImplicitCast& impl_cast) {
    out << indent() << "ImplicitCast: (" << type_str(impl_cast.type) << ")\n";
    visit(impl_cast.target);
}

void JLDumper::visit_ExplicitCast(ExplicitCast& expl_cast) {
    out << indent() << "ExplicitCast: (" << type_str(expl_cast.type) << ")\n";
    visit(expl_cast.target);
}

void JLDumper::visit_DeclRefExpr(DeclRefExpr& dre) {
    out << indent() << "DeclRefExpr: ";

    Expr* node = ctx.get_node(dre.decl);
    if (auto* decl = dyn_cast<Decl>(node)) {
        out << "resolved, '" << sym(decl->identifier) << "' (" << decl_type_str(decl) << '@'
            << std::to_string(dre.decl.id_value()) << ", " << type_str(decl->type) << ")\n";

        if (isa<OpaqueFunction>(decl)) {
            out << indent() << dump_label("opaque function");
            inc_indent();
            visit(dre.decl);
            dec_indent();
        }
    } else if (auto* sym_lit = dyn_cast<SymbolLiteral>(node)) {
        out << "unresolved, :(" << sym(sym_lit->value) << ")\n";
    } else {
        assert(false && "decl ref expr pointing to non-decl, non-symbol node type");
    }
}

void JLDumper::visit_Assignment(Assignment& assign) {
    out << indent() << "Assignment" << (assign.is_implicit_decl() ? " (with implicit decl)" : "")
        << ": \n";

    if (assign.is_implicit_decl()) {
        out << indent() << dump_label("implicit decl");
        inc_indent();

        const auto* dre = ctx.get_and_dyn_cast<DeclRefExpr>(assign.target);
        visit(dre->decl);

        dec_indent();
    }

    out << indent() << dump_label("target");
    inc_indent();
    visit(assign.target);
    dec_indent();

    out << indent() << dump_label("value");
    inc_indent();
    visit(assign.value);
    dec_indent();
}

void JLDumper::visit_UpdateAssignment(UpdateAssignment& up_assign) {
    out << indent()
        << fmt::format("UpdateAssignment (broadcast: {}):\n",
                       up_assign.is_broadcast() ? "yes" : "no");

    dump_with_label("update_fn", up_assign.update_fn);
    dump_with_label("target", up_assign.target);
    dump_with_label("value", up_assign.value);
}

void JLDumper::visit_FunctionCall(FunctionCall& fn_call) {
    out << indent()
        << fmt::format("FunctionCall (broadcast: {}):\n", fn_call.is_broadcast() ? "yes" : "no");

    dump_with_label("target_fn", fn_call.target_fn);
    dump_with_label("args", fn_call.args);
}

void JLDumper::visit_LogicalBinOp(LogicalBinOp& lbo) {
    out << indent() << "LogicalBinOp (" << (lbo.is_land() ? "&&" : "||") << "):\n";

    dump_with_label("lhs", lbo.lhs);
    dump_with_label("rhs", lbo.rhs);
}

void JLDumper::visit_IfExpr(IfExpr& if_) {
    out << indent() << "IfExpr:\n";

    dump_with_label("condition", if_.condition);
    dump_with_label("true_branch", if_.true_branch);
    dump_with_label("false_branch", if_.false_branch);
}

void JLDumper::visit_WhileExpr(WhileExpr& while_) {
    out << indent() << "WhileExpr:\n";

    dump_with_label("condition", while_.condition);
    dump_with_label("body", while_.body);
}

void JLDumper::visit_ReturnStmt(ReturnStmt& return_stmt) {
    out << indent() << "ReturnStmt:\n";

    inc_indent();
    visit(return_stmt.inner);
    dec_indent();
}

void JLDumper::visit_ContinueStmt([[maybe_unused]] ContinueStmt& cont_stmt) {
    out << indent() << "ContinueStmt\n";
}

void JLDumper::visit_BreakStmt([[maybe_unused]] BreakStmt& break_stmt) {
    out << indent() << "BreakStmt\n";
}
}; // namespace stc::jl
