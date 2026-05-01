#include "ast/context.h"
#include "ast/visitor.h"
#include "common/utils.h"
#include <catch2/catch_test_macros.hpp>

using namespace stc;

struct TestId : public SplitU32Id {
    using SplitU32Id::SplitU32Id;

    static constexpr TestId null_id() { return TestId{0U, 0U}; }

    bool is_null() const { return false; }
};

enum class DummyKind : uint8_t { Base = 0, Expr = 1, Stmt = 2 };

struct DummyNodeBase {
    using kind_type = DummyKind;
    DummyKind _kind;

    explicit DummyNodeBase(DummyKind k)
        : _kind(k) {}
    virtual ~DummyNodeBase() = default;

    DummyKind kind() const { return _kind; }

    static DummyNodeBase* safe_cast_to_base(void* ptr, TestId) {
        return static_cast<DummyNodeBase*>(ptr);
    }
};

struct DummyExpr : DummyNodeBase {
    int value;
    static bool same_node_kind(DummyKind k) { return k == DummyKind::Expr; }
    DummyExpr(int v)
        : DummyNodeBase(DummyKind::Expr), value(v) {}
};

struct DummyStmt : DummyNodeBase {
    static bool same_node_kind(DummyKind k) { return k == DummyKind::Stmt; }
    DummyStmt()
        : DummyNodeBase(DummyKind::Stmt) {}
};

class DummyCtx : public ASTCtx<TestId, DummyNodeBase> {
public:
    using ASTCtx<TestId, DummyNodeBase>::ASTCtx;
};

TEST_CASE("node emplace and retrieval", "[ASTCtx]") {
    DummyCtx ctx;

    SECTION("valid init and id returned") {
        auto [id, expr_ptr] = ctx.emplace_node<DummyExpr>(42);

        REQUIRE_FALSE(id.is_null());
        REQUIRE(id.kind_value() == static_cast<uint8_t>(DummyKind::Expr));
        REQUIRE(expr_ptr->value == 42);
    }

    SECTION("get_node") {
        auto [id, expr_ptr] = ctx.emplace_node<DummyExpr>(99);

        DummyNodeBase* retrieved = ctx.get_node(id);
        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved == expr_ptr);
        REQUIRE(retrieved->kind() == DummyKind::Expr);
    }

    SECTION("isa and get_and_dyn_cast") {
        auto [id, expr_ptr] = ctx.emplace_node<DummyExpr>(100);

        REQUIRE(ctx.isa<DummyExpr>(id));
        REQUIRE_FALSE(ctx.isa<DummyStmt>(id));

        DummyExpr* casted = ctx.get_and_dyn_cast<DummyExpr>(id);
        REQUIRE(casted != nullptr);
        REQUIRE(casted->value == 100);

        DummyStmt* bad_cast = ctx.get_and_dyn_cast<DummyStmt>(id);
        REQUIRE(bad_cast == nullptr);
    }
}

class TestVisitor : public ASTVisitor<TestVisitor, DummyCtx, int> {
public:
    int visit_count          = 0;
    bool skip_next_pre_visit = false;

    using ASTVisitor::ASTVisitor;

    // overwrites base dispatch for now
    int dispatch(DummyNodeBase* node) {
        visit_count++;
        if (node->kind() == DummyKind::Expr) {
            return static_cast<DummyExpr*>(node)->value;
        }
        return visit_default_case();
    }

    int visit_default_case() { return -1; }

    bool pre_visit_ptr([[maybe_unused]] DummyNodeBase* node) {
        if (skip_next_pre_visit) {
            skip_next_pre_visit = false;
            return false;
        }

        return true;
    }
};

TEST_CASE("CRTP traversal", "ASTVisitor") {
    DummyCtx ctx;
    auto [expr_id, expr_ptr] = ctx.emplace_node<DummyExpr>(55);
    auto [stmt_id, stmt_ptr] = ctx.emplace_node<DummyStmt>();

    TestVisitor visitor{ctx};

    SECTION("dispatch from ptr") {
        int result = visitor.visit(expr_ptr);
        REQUIRE(result == 55);
        REQUIRE(visitor.visit_count == 1);
    }

    SECTION("dispatch from node id") {
        int result = visitor.visit(expr_id);
        REQUIRE(result == 55);
    }

    SECTION("default visitor") {
        int result = visitor.visit(stmt_ptr);
        REQUIRE(result == -1);
    }

    SECTION("pre-visit cancel") {
        visitor.skip_next_pre_visit = true;

        int result = visitor.visit(expr_ptr);

        REQUIRE(result == -1);
        REQUIRE(visitor.visit_count == 0);
    }
}
