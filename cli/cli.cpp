#include <julia_guard.h>
JULIA_DEFINE_FAST_TLS

#include <api/transpiler.h>
#include <common/config.h>
#include <common/term_utils.h>
#include <common/utils.h>
#include <frontend/jl/utils.h>

#include "cli_utils.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace stc::cli {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
int run(int argc, char* argv[]) {
    using namespace stc;

    if (argc < 2) {
        std::cerr << stc::colored("not enough arguments\n", ansi_codes::red);
        std::cerr << "usage: stc <input file> [OPTIONS]\n";
        std::cerr << "run with " << stc::colored("--help", ansi_codes::yellow) << " or "
                  << stc::colored("-h", ansi_codes::yellow) << " for a list of available options\n";
        return 1;
    }

    // only help or version info can be invoked on their own
    // for every other case, arguments must start with the input path

    std::string argv1{argv[1]};

    if (argv1 == "-h" || argv1 == "--help") {
        print_help();
        return 0;
    }

    if (argv1 == "-v" || argv1 == "--version") {
        print_version_info();
        return 0;
    }

    std::string path = argv[1];

    TranspilerConfig config{};
    std::string out_path{};
    uint16_t ite_count = 1U;
    bool run_benchmark = true;
    bool no_out        = false;

    for (int i = 2; i < argc; i++) {
        std::string arg{argv[i]};

        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        }

        if (arg == "-v" || arg == "--version") {
            print_version_info();
            return 0;
        }

        if (arg == "--no-benchmark")
            run_benchmark = false;
        else if (arg == "--dump-parsed")
            config.dump_parsed = true;
        else if (arg == "--dump-sema")
            config.dump_sema = true;
        else if (arg == "--dump-lowered")
            config.dump_lowered = true;
        else if (arg == "--dump-scopes")
            config.dump_scopes = true;
        else if (arg == "--spaces")
            config.use_tabs = false;
        else if (arg == "--tabs")
            config.use_tabs = true;
        else if (arg == "--no-out")
            no_out = true;
        else if (arg == "--fwd-fns")
            config.forward_fns = true;
        else if (arg == "--conv-fail-reason")
            config.print_convert_fail_reason = true;
        else if (arg == "--track-bindings")
            config.track_bindings = true;
        else if (arg == "--no-coerce-i32")
            config.coerce_to_i32 = false;
        else if (arg == "--no-coerce-f32")
            config.coerce_to_f32 = false;
        else if (arg == "--no-coercion") {
            config.coerce_to_i32 = false;
            config.coerce_to_f32 = false;
        } else if (arg == "--no-uniform-capture")
            config.capture_uniforms = false;
        else if (arg == "-Wfwd-fns")
            config.warn_on_fn_forward = true;
        else if (arg == "-Wjl-query")
            config.warn_on_jl_sema_query = true;
        else if (arg == "--err-dump") {
            if (i + 1 >= argc) {
                std::cerr << "--err-dump must be followed by a verbosity level\n";
                return 1;
            }

            std::string_view verbosity{argv[i + 1]};

            if (verbosity == "none")
                config.err_dump_verbosity = DumpVerbosity::None;
            else if (verbosity == "partial")
                config.err_dump_verbosity = DumpVerbosity::Partial;
            else if (verbosity == "verbose")
                config.err_dump_verbosity = DumpVerbosity::Verbose;
            else {
                std::cerr << fmt::format("'{}' is not a valid error dumping verbosity level "
                                         "(options: none, partial, verbose)\n",
                                         verbosity);
                return 1;
            }

            i++;
        } else if (arg == "--it") {
            if (i + 1 >= argc) {
                std::cerr << "--it must be followed by the number of iterations\n";
                return 1;
            }

            std::string next_arg{argv[i + 1]};
            auto maybe_ite_count = try_parse_u16(next_arg);

            if (!maybe_ite_count.has_value()) {
                std::cerr << "--it followed by a non-numeric string\n";
                return 1;
            }

            ite_count = *maybe_ite_count;
            i++;
        } else if (arg == "--cg-indent") {
            if (i + 1 >= argc) {
                std::cerr
                    << "--cg-indent must be followed by the width of indentation (in spaces)\n";
                return 1;
            }

            auto maybe_cg_indent = try_parse_u16(std::string{argv[i + 1]});

            if (!maybe_cg_indent.has_value()) {
                std::cerr << "--cg-indent followed by a non-numeric string\n";
                return 1;
            }

            config.code_gen_indent = *maybe_cg_indent;
            i++;
        } else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "-o must be followed by the output file's path\n";
                return 1;
            }

            out_path = argv[i + 1];
            i++;
        } else if (arg == "--gl-version") {
            if (i + 1 >= argc) {
                std::cerr << "--gl-version must be followed by the desired OpenGL version\n";
                return 1;
            }

            config.target_version = std::string{argv[i + 1]};
            i++;
        } else {
            std::cerr << fmt::format("unknown argument: {}\n", arg);
            return 1;
        }
    }

    if (no_out && !out_path.empty()) {
        std::cerr << stc::colored("warning: ", ansi_codes::yellow)
                  << "conflicting flags have been provided as arguments: "
                  << stc::colored("--no-out", ansi_codes::cyan) << " and "
                  << stc::colored("-o", ansi_codes::cyan) << ' '
                  << stc::colored(out_path, ansi_codes::cyan) << ", "
                  << "no output will be created after transpilation.\n";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "couldn't open input file at '" << path << "'\n";
        return 1;
    }

    std::stringstream code_stream;
    code_stream << file.rdbuf();
    std::string code{code_stream.str()};

#define STC_CHECK_EXCEPTIONS                                                                       \
    if (jl::check_exceptions()) {                                                                  \
        std::cerr << "\nan error occured while initializing Julia\n";                              \
        return 1;                                                                                  \
    }

#define STC_EVAL_AND_CHECK(str)                                                                    \
    jl_eval_string(str);                                                                           \
    STC_CHECK_EXCEPTIONS

    std::cout << "initializing Julia environment for transpilation...\n";

    jl_init();
    STC_CHECK_EXCEPTIONS

    STC_EVAL_AND_CHECK("using JuliaGLM");

    std::cout << "starting transpilation...\n";

    const ScopeGuard jl_guard{[&]() { jl_atexit_hook(0); }};

    for (size_t i = 0; i < ite_count; i++) {
        if (run_benchmark && ite_count != 1) {
            std::string header_title{fmt::format("  Transpilation #{}  ", i + 1)};
            std::string sep(header_title.size(), '=');

            std::cout << '\n' << sep << '\n';
            std::cout << header_title << '\n';
            std::cout << sep << '\n';
        }

        auto result = run_benchmark
                          ? api::transpile<true>(code, tail_from_cwd(path).string(), config)
                          : api::transpile<false>(code, tail_from_cwd(path).string(), config);

        if (!result.has_value()) {
            std::cerr << "\nan error occured during transpilation\n";
            return 1;
        }

        if (!no_out && i + 1 == ite_count) {
            if (out_path.empty()) {
                std::cout << "Output of last transpilation:\n\n";
                std::cout << *result << '\n';
                break;
            }

            // gotta do C-style file writing, because libjulia messes with std::locale in a way that
            // breaks std::ofstream in some release builds

            stc::FileRAII out_file{out_path, "w"};
            if (out_file.get() == nullptr) {
                std::cerr << "\nfailed to open '" << out_path << "' for writing (errno: " << errno
                          << '\n';
                return 1;
            }

            size_t written = fwrite(result->data(), 1, result->size(), out_file.get());
            if (written != result->size()) {
                std::cerr << "\ncouldn't write output to file\n";
                return 1;
            }
        }
    }

    return 0;

#undef STC_EVAL_AND_CHECK
#undef STC_CHECK_EXCEPTIONS
}

} // namespace stc::cli

int main(int argc, char* argv[]) {
    try {
        return stc::cli::run(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "an unexpected error occured while running stc:\n";
        std::cerr << ex.what() << '\n';
    } catch (...) {
        std::cerr << "an unexpected error occured while running stc\n";
    }

    return 1;
}
