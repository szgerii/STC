#pragma once

#include "api/benchmark_tracker.h"
#include "common/config.h"
#include "frontend/jl/ast.h"
#include "frontend/jl/context.h"

#include <optional>
#include <string>
#include <string_view>

namespace stc::api {

// NOTE
// the cpp transpile calls are only publicly exported
// because of mixing std lib with dll exporting, they're meant to be used only if linking project is
// also being built under identical conditions to the library (e.g. the CLI)
// otherwise, prefer the C-level API

using MaybeString = std::optional<std::string>;

template <bool RunBenchmark>
STC_API MaybeString transpile_parsed(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                                     detail::BenchmarkTracker<RunBenchmark>& benchmark_tracker);

template <bool RunBenchmark>
STC_API MaybeString transpile(std::string_view code, std::optional<std::string_view> file_path,
                              stc::TranspilerConfig config,
                              std::string_view juliaglm_path = "Main.JuliaGLM");

template <bool RunBenchmark>
STC_API MaybeString transpile(jl_value_t* expr_v, stc::TranspilerConfig config,
                              std::string_view juliaglm_path = "Main.JuliaGLM");

extern "C" {
    STC_API uint8_t stc_abi_version() noexcept;

    // set cfg_handle to NULL to use default configuration settings
    STC_API void* stc_transpile(jl_value_t* expr_v, bool run_benchmark, void* cfg_handle) noexcept;

    STC_API const char* stc_get_result(void* result_handle) noexcept;
    STC_API void stc_free_result(void* result_handle) noexcept;
}

}; // namespace stc::api
