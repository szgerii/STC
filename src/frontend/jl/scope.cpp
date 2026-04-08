#include "frontend/jl/scope.h"
#include "frontend/jl/dumper.h"

namespace {

using namespace stc::jl;

std::string scope_kind_str(ScopeKind kind) {
    switch (kind) {
        case ScopeKind::Global:
            return "global";

        case ScopeKind::Hard:
            return "hard";

        case ScopeKind::Soft:
            return "soft";
    }

    throw std::logic_error{"Unaccounted ScopeKind value in scope_kind_str"};
}

std::string binding_type_str(BindingType bt) {
    switch (bt) {
        case BindingType::Global:
            return "global";

        case BindingType::Captured:
            return "captured";

        case BindingType::Local:
            return "local";
    }

    throw std::logic_error{"Unaccounted BindingType value in binding_type_str"};
}

std::string lft_entry_state_str(LFTEntry::State state) {
    switch (state) {
        case LFTEntry::State::Unresolved:
            return "unresolved";

        case LFTEntry::State::InProgress:
            return "in progress";

        case LFTEntry::State::Resolved:
            return "resolved";
    }

    throw std::logic_error{"Unaccounted LFTEntry::State value in lft_entry_state_str"};
}

} // namespace

namespace stc::jl {

void JLScope::dump(const JLCtx& ctx, std::ostream& out) const {
    // CLEANUP: indents
    std::string single_indent{indent(1, ctx.config.dump_indent, ctx.config.use_tabs)};
    std::string double_indent{single_indent + single_indent};

    out << "==============================\n";
    out << "scope snapshot debug dump\n\n";

    out << std::format("kind: {}, depth: {}\n\n", scope_kind_str(kind), depth());

    out << "binding table:\n";

    for (auto [sym_id, bt] : binding_table)
        out << single_indent
            << std::format("{} -> {}\n", ctx.get_sym(sym_id), binding_type_str(bt));

    out << "\n\nsymbol table:\n";

    JLDumper dumper{ctx, std::cout};

    for (auto [sym_id, decl_id] : symbol_table) {
        out << single_indent << std::format("{} -> \n", ctx.get_sym(sym_id));
        dumper.visit(decl_id);
    }

    out << "\n\nlocal function table:\n";

    for (const auto& [sym_id, entry] : local_fn_table) {
        out << single_indent << ctx.get_sym(sym_id) << '(' << lft_entry_state_str(entry.state)
            << ") ->\n";

        for (const auto* mdecl : entry.method_decls) {
            if (mdecl == nullptr) {
                out << double_indent << "nullptr\n";
                continue;
            }

            out << double_indent << ctx.get_sym(mdecl->identifier) << '@'
                << std::to_string(ctx.calculate_node_id(*mdecl)) << '\n';
        }
    }

    out << "\n\ndeferred methods:\n";

    for (NodeId method_decl : deferred_method_queue)
        dumper.visit(method_decl);

    out << "\n==============================\n";
}

} // namespace stc::jl
