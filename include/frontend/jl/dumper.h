#pragma once

#include <iostream>

#include "common/config.h"
#include "frontend/jl/visitor.h"

namespace stc::jl {

class JLDumper final : public JLVisitor<JLDumper, const JLCtx, void> {
public:
    explicit JLDumper(const JLCtx& ctx, std::ostream& out)
        : JLVisitor{ctx}, out{out} {}

    bool pre_visit_id(NodeId node);
    bool pre_visit_ptr(Expr* expr);

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "frontend/jl/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::ostream& out;
    size_t indent_level = 0U;

    std::string type_str(TypeId type_id) const;
    std::string_view sym(SymbolId sym_id) const;
    std::string indent() const;
    void inc_indent();
    void dec_indent();

    void dump_with_label(std::string_view label, NodeId node);
    void dump_with_label(std::string_view label, const std::vector<NodeId>& nodes);
};

static_assert(CJLVisitorImpl<JLDumper, void>);

} // namespace stc::jl
