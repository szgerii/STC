#pragma once

#include <cstdint>
#include <string>

namespace stc {

// None    -> nothing is dumped when an error is encountered
// Partial -> the affected subtree of the AST is dumped when an error is encountered
// Verbose -> the entire AST is dumped when an error is encountered
enum class DumpVerbosity : uint8_t { First = 0, None = First, Partial, Verbose, Last = Verbose };

struct TranspilerConfig {
    uint16_t code_gen_indent         = 4;
    uint16_t dump_indent             = 2;
    DumpVerbosity err_dump_verbosity = DumpVerbosity::None;
    bool use_tabs                    = false;
    bool dump_scopes                 = false;
    bool forward_fns                 = true;
    bool warn_on_fn_forward          = false;
    bool warn_on_jl_sema_query       = false;
    bool print_convert_fail_reason   = false;
    bool track_bindings              = false;
    bool coerce_to_f32               = true;
    bool coerce_to_i32               = true;
    bool capture_uniforms            = true;
    bool dump_parsed                 = false;
    bool dump_sema                   = false;
    bool dump_lowered                = false;
    std::string target_version       = "460";
};

} // namespace stc
