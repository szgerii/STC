#include "api/transpiler.h"

#include "backend/glsl/code_gen.h"
#include "backend/glsl/target_info.h"
#include "frontend/jl/dumper.h"
#include "frontend/jl/lowering.h"
#include "frontend/jl/parser.h"
#include "frontend/jl/sema.h"
#include "meta.h"
#include "sir/dumper.h"

namespace stc::api {

template <bool RunBenchmark>
MaybeString transpile_parsed(jl::NodeId jl_ast, jl::JLCtx& jl_ctx,
                             detail::BenchmarkTracker<RunBenchmark>& benchmark_tracker) {
    using namespace stc::jl;

    auto* cmpd = jl_ctx.get_and_dyn_cast<CompoundExpr>(jl_ast);
    if (cmpd == nullptr) {
        std::cerr << "outermost AST node is not a compound expression\n";
        return std::nullopt;
    }

    benchmark_tracker.sema_start();

    JLSema sema{jl_ctx, *cmpd};
    sema.infer(jl_ast);
    sema.finalize();

    benchmark_tracker.sema_end();

    if (!sema.success()) {
        std::cerr << "\nJulia sema failed\n";
        return std::nullopt;
    }

    if (jl_ctx.config.dump_sema) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    benchmark_tracker.lowering_start();

    JLLoweringVisitor lowering{std::move(jl_ctx)};
    auto sir_ast = lowering.lower(jl_ast);

    if (!lowering.success()) {
        std::cerr << "\nJulia -> SIR lowering failed\n";
        return std::nullopt;
    }

    auto glsl_ctx = glsl::GLSLCtx(std::move(lowering.sir_ctx));

    benchmark_tracker.lowering_end();

    if (glsl_ctx.config.dump_lowered) {
        sir::SIRDumper dumper{glsl_ctx, std::cout};
        dumper.visit(sir_ast);
    }

    benchmark_tracker.code_gen_start();

    glsl::GLSLCodeGenVisitor code_gen_vis{glsl_ctx};
    code_gen_vis.visit(sir_ast);

    if (!code_gen_vis.success()) {
        std::cerr << "\nGLSL code gen failed\n";
        return std::nullopt;
    }

    benchmark_tracker.code_gen_end();
    benchmark_tracker.end();

    benchmark_tracker.print(std::cout);

    return code_gen_vis.move_result();
}

template STC_API MaybeString transpile_parsed<true>(jl::NodeId, jl::JLCtx&,
                                                    detail::BenchmarkTracker<true>&);
template STC_API MaybeString transpile_parsed<false>(jl::NodeId, jl::JLCtx&,
                                                     detail::BenchmarkTracker<false>&);

template <bool RunBenchmark>
MaybeString transpile(std::string_view code, std::optional<std::string_view> file_path,
                      stc::TranspilerConfig config, std::string_view juliaglm_path) {
    using namespace stc::jl;

    detail::BenchmarkTracker<RunBenchmark> benchmark_tracker{};

    benchmark_tracker.start();
    benchmark_tracker.init_start();

    JLCtx jl_ctx{juliaglm_path};
    jl_ctx.config = std::move(config);

    glsl::GLSLTargetInfo target_info{jl_ctx.type_pool};
    jl_ctx.target_info = &target_info;

    benchmark_tracker.init_end();
    benchmark_tracker.parser_start();

    JLParser parser{jl_ctx};

    if (file_path.has_value())
        parser.fallback_file = *file_path;

    NodeId jl_ast = parser.parse_code(code);

    if (!parser.success()) {
        std::cerr << "\nJulia parser failed\n";
        return std::nullopt;
    }

    benchmark_tracker.parser_end();

    if (jl_ctx.config.dump_parsed) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    return transpile_parsed<RunBenchmark>(jl_ast, jl_ctx, benchmark_tracker);
}

template STC_API MaybeString transpile<true>(std::string_view, std::optional<std::string_view>,
                                             stc::TranspilerConfig, std::string_view);
template STC_API MaybeString transpile<false>(std::string_view, std::optional<std::string_view>,
                                              stc::TranspilerConfig, std::string_view);

template <bool RunBenchmark>
MaybeString transpile(jl_value_t* expr_v, stc::TranspilerConfig config,
                      std::string_view juliaglm_path) {
    using namespace stc::jl;

    detail::BenchmarkTracker<RunBenchmark> benchmark_tracker{};

    benchmark_tracker.start();
    benchmark_tracker.init_start();

    JLCtx jl_ctx{juliaglm_path};
    jl_ctx.config = std::move(config);

    glsl::GLSLTargetInfo target_info{jl_ctx.type_pool};
    jl_ctx.target_info = &target_info;

    benchmark_tracker.init_end();
    benchmark_tracker.parser_start();

    JLParser parser{jl_ctx};

    NodeId jl_ast = parser.parse(expr_v);

    if (!parser.success()) {
        std::cerr << "\nJulia parser failed\n";
        return std::nullopt;
    }

    benchmark_tracker.parser_end();

    if (jl_ctx.config.dump_parsed) {
        JLDumper dumper{jl_ctx, std::cout};
        dumper.visit(jl_ast);
    }

    return transpile_parsed<RunBenchmark>(jl_ast, jl_ctx, benchmark_tracker);
}

template STC_API MaybeString transpile<true>(jl_value_t*, stc::TranspilerConfig, std::string_view);
template STC_API MaybeString transpile<false>(jl_value_t*, stc::TranspilerConfig, std::string_view);

namespace {

std::string* do_transpile(jl_value_t* expr_v, bool run_benchmark, stc::TranspilerConfig& config) {
    using namespace stc;
    using namespace stc::jl;

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto get_failure_result = []() { return new std::string{}; };

    if (!jl_is_expr(expr_v)) {
        std::cerr << "received non-Expr value from Julia\n";
        return get_failure_result();
    }

    std::optional<std::string> result = run_benchmark ? api::transpile<true>(expr_v, config)
                                                      : api::transpile<false>(expr_v, config);

    if (!result.has_value())
        return get_failure_result();

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new std::string(std::move(*result));
}

} // namespace

extern "C" {
    STC_API uint8_t stc_abi_version() noexcept {
        return stc::meta::version_major;
    }

    // ! this returns a handle ptr, which must be read through stc_jl_cstr_from_handle and destroyed
    // ! with free_handle
    STC_API void* stc_transpile(jl_value_t* expr_v, bool run_benchmark, void* cfg_handle) noexcept {
        if (!jl_is_expr(expr_v)) {
            std::cerr << "non-Expr julia object cannot be transpiled\n";
            return nullptr;
        }

        try {

            TranspilerConfig config = cfg_handle != nullptr
                                          ? *(static_cast<TranspilerConfig*>(cfg_handle))
                                          : TranspilerConfig{};

            std::string* result_mem = do_transpile(expr_v, run_benchmark, config);
            return static_cast<void*>(result_mem);

        } catch (const std::exception& ex) {
            std::cerr << "the following std::exception was thrown during transpilation:\n";
            std::cerr << ex.what() << '\n';
        } catch (...) {
            std::cerr << "an unexpected error was thrown during transpilation\n";
        }

        return nullptr;
    }

    // gets the underlying C string data of a result handle
    STC_API const char* stc_get_result(void* result_handle) noexcept {
        if (result_handle == nullptr) {
            std::cerr << "nullptr passed to stc_get_result\n";
            return nullptr;
        }

        return static_cast<std::string*>(result_handle)->c_str();
    }

    // frees the underlying string data belonging to a result handle
    STC_API void stc_free_result(void* result_handle) noexcept {
        if (result_handle == nullptr) {
            std::cerr << "nullptr passed to stc_free_result\n";
            return;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete static_cast<std::string*>(result_handle);
    }
}

} // namespace stc::api
