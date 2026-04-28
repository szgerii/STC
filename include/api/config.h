#pragma once

#include "base.h"

#include <cstdint>
#include <stdexcept>

namespace stc::api {

// C-level API for handling configuration
extern "C" {
    STC_API void* stc_create_cfg() noexcept;
    STC_API void stc_free_cfg(void* cfg_handle) noexcept;
    STC_API void stc_set_code_gen_indent(void* cfg_handle, uint16_t value) noexcept;
    STC_API void stc_set_dump_indent(void* cfg_handle, uint16_t value) noexcept;

    // 0 - None, 1 - Partial, 2 - Verbose
    STC_API void stc_set_err_dump_verbosity(void* cfg_handle, uint8_t value) noexcept;

    STC_API void stc_set_use_tabs(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_scopes(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_forward_fns(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_warn_on_fn_forward(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_warn_on_jl_sema_query(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_print_convert_fail_reason(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_track_bindings(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_coerce_to_f32(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_coerce_to_i32(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_capture_uniforms(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_parsed(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_sema(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_dump_lowered(void* cfg_handle, bool value) noexcept;
    STC_API void stc_set_target_version(void* cfg_handle, const char* value) noexcept;
}

} // namespace stc::api
