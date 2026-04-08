#pragma once

#include <iostream>

#include "sir/visitor.h"

namespace stc::sir {

class SIRDumper final : public SIRVisitor<SIRDumper, SIRCtx, void> {
public:
    explicit SIRDumper(SIRCtx& ctx, std::ostream& out)
        : SIRVisitor{ctx}, out{out} {}

    bool pre_visit_id(NodeId node);

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "sir/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::ostream& out;
    size_t indent_level = 0U;

    std::string type_str(TypeId type_id) const;
    std::string indent() const;
    void inc_indent();
    void dec_indent();
};

static_assert(CSIRVisitorImpl<SIRDumper, void>);

} // namespace stc::sir
