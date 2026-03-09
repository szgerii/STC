#pragma once

#include <sstream>

#include "backends/glsl/glsl_context.h"
#include "ir/ast_visitor.h"

namespace stc::glsl {

class GLSLCodeGenVisitor : public ASTVisitor<GLSLCodeGenVisitor, void, const GLSLCtx> {
public:
    GLSLCodeGenVisitor(const GLSLCtx& ctx)
        : ASTVisitor{ctx} {}

    std::string result() const { return out.str(); }
    bool success() const { return successful_gen; }

    // clang-format off
    #define X(type, kind) STC_AST_VISITOR_DECL(void, type)
        #include "ir/node_defs/all_nodes.def"
    #undef X
    // clang-format on

private:
    std::stringstream out{};
    size_t indent_level = 0U;
    bool successful_gen = true;

    std::string indent() const;
};

} // namespace stc::glsl
