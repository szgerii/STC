#pragma once

#include "common/bump_arena.h"
#include "ir/ast.h"
#include "ir/type_pool.h"

namespace stc::ir {

struct ASTCtx {
private:
    BumpArena<NodeId::id_type> node_arena;            // currently: BumpArena32
    BumpArena<SrcLocationId::id_type> src_info_arena; // currently: BumpArena32
    BumpArena<TypeId::id_type> type_arena;            // currently: BumpArena16

public:
    TypePool type_pool;
    SrcInfoManager src_info_manager;

    explicit ASTCtx()
        : node_arena{128 * 1024},
          src_info_arena{128 * 1024},
          type_arena{32 * 1024},
          type_pool{type_arena},
          src_info_manager{src_info_arena} {}

    template <CNodeTy T, typename... Args>
    std::pair<NodeId, T*> alloc_node(Args&&... args) {
        return node_arena.emplace<T>(std::forward<Args>(args)...);
    }

    inline NodeBase* get_node(NodeId id) const {
        return static_cast<NodeBase*>(node_arena.get_ptr(id));
    }

    template <typename T>
    inline T* get_dyn(NodeId id) const {
        return dyn_cast<T>(get_node(id));
    }

    inline Decl* get_decl(NodeId id) const { return get_dyn<Decl>(id); }
    inline Stmt* get_stmt(NodeId id) const { return get_dyn<Stmt>(id); }
    inline Expr* get_expr(NodeId id) const { return get_dyn<Expr>(id); }

    // helper, mainly meant for debug assertions
    template <typename T>
    bool isa(NodeId id) const {
        if (id == NodeId::null_id())
            return false;

        NodeBase* node = static_cast<NodeBase*>(node_arena.get_ptr(id));
        assert(node != nullptr);

        return dyn_cast<T>(node) != nullptr;
    }

    operator const TypePool&() const { return type_pool; }
    operator const SrcInfoManager&() const { return src_info_manager; }
};

}; // namespace stc::ir
