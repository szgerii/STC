#include "api/config.h"

#include "common/config.h"

namespace stc::api {
extern "C" {
    STC_API void* stc_create_cfg() noexcept {
        try {
            return new TranspilerConfig{}; // NOLINT(cppcoreguidelines-owning-memory)
        } catch (...) {
            return nullptr;
        }
    }

    STC_API void stc_free_cfg(void* cfg_handle) noexcept {
        if (cfg_handle != nullptr) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete static_cast<TranspilerConfig*>(cfg_handle);
        }
    }

#define STC_SET_CFG_VALUE(handle, field, value)                                                    \
    if ((handle) != nullptr) {                                                                     \
        static_cast<TranspilerConfig*>(handle)->field = (value);                                   \
    }

    STC_API void stc_set_code_gen_indent(void* cfg_handle, uint16_t value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, code_gen_indent, value);
    }

    STC_API void stc_set_dump_indent(void* cfg_handle, uint16_t value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_indent, value);
    }

    STC_API void stc_set_err_dump_verbosity(void* cfg_handle, uint8_t value) noexcept {
        if (value < static_cast<uint8_t>(DumpVerbosity::First) ||
            value > static_cast<uint8_t>(DumpVerbosity::Last))
            return;

        STC_SET_CFG_VALUE(cfg_handle, err_dump_verbosity, static_cast<DumpVerbosity>(value));
    }

    STC_API void stc_set_use_tabs(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, use_tabs, value);
    }

    STC_API void stc_set_dump_scopes(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_scopes, value);
    }

    STC_API void stc_set_forward_fns(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, forward_fns, value);
    }

    STC_API void stc_set_warn_on_fn_forward(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, warn_on_fn_forward, value);
    }

    STC_API void stc_set_warn_on_jl_sema_query(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, warn_on_jl_sema_query, value);
    }

    STC_API void stc_set_print_convert_fail_reason(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, print_convert_fail_reason, value);
    }

    STC_API void stc_set_track_bindings(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, track_bindings, value);
    }

    STC_API void stc_set_coerce_to_f32(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, coerce_to_f32, value);
    }

    STC_API void stc_set_coerce_to_i32(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, coerce_to_i32, value);
    }

    STC_API void stc_set_capture_uniforms(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, capture_uniforms, value);
    }

    STC_API void stc_set_dump_parsed(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_parsed, value);
    }

    STC_API void stc_set_dump_sema(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_sema, value);
    }

    STC_API void stc_set_dump_lowered(void* cfg_handle, bool value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, dump_lowered, value);
    }

    STC_API void stc_set_target_version(void* cfg_handle, const char* value) noexcept {
        STC_SET_CFG_VALUE(cfg_handle, target_version, value != nullptr ? std::string{value} : "");
    }

#undef STC_SET_CFG_VALUE
}
} // namespace stc::api
