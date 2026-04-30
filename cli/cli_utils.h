#pragma once

#include <common/term_utils.h>
#include <meta.h>

#include <array>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace stc::cli {

inline std::filesystem::path tail_from_cwd(const std::filesystem::path& p) {
    namespace fs = std::filesystem;

    fs::path abs = fs::absolute(p);
    fs::path cwd = fs::current_path();

    fs::path rel = abs.lexically_relative(cwd);

    if (!rel.empty())
        return rel;

    return abs;
}

inline std::optional<std::uint16_t> try_parse_u16(const std::string& str) {
    uint16_t value  = 0;
    auto [ptr, err] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (err == std::errc() && ptr == str.data() + str.size())
        return value;

    return std::nullopt;
}

inline void print_help() {
    static constexpr auto title_col = ansi_codes::cyan;
    static constexpr auto flag_col  = ansi_codes::yellow;

    // clang-format off
    std::cout << stc::colored("usage:", title_col) << " stc <input file> [OPTIONS]\n\n";

    std::cout << stc::colored("general:\n", title_col);
    std::cout << "  " << stc::colored("-h, --help",         flag_col)        << "           show this help message and exit\n";
    std::cout << "  " << stc::colored("-v, --version",      flag_col)        << "        show version information and exit\n";
    std::cout << "  " << stc::colored("-o <path>",          flag_col)        << "            set output file path (default: print to console only)\n";
    std::cout << "  " << stc::colored("--no-out",           flag_col)        << "             do not output the generated code to the console, or to disk\n";
    std::cout << "  " << stc::colored("--gl-version <ver>", flag_col)        << "   set OpenGL version for #version directive (default: \"460\")\n";
    std::cout << "  " << stc::colored("--it <n>",           flag_col)        << "             run transpilation N times (for benchmarking)\n";
    std::cout << "  " << stc::colored("--no-benchmark",     flag_col)        << "       disable the measuring and printing of a transpilation time breakdown\n";
    std::cout << '\n';
    
    std::cout << stc::colored("transpilation behavior:\n", title_col);
    std::cout << "  " << stc::colored("--conv-fail-reason", flag_col)   << "   print reason for conversion/casting failures\n";
    std::cout << "  " << stc::colored("--no-coerce-i32",    flag_col)   << "      disable automatic Int64 -> Int32 literal type coercion\n";
    std::cout << "  " << stc::colored("--no-coerce-f32",      flag_col)   << "      disable automatic Float64 -> Float32 literal type coercion\n";
    std::cout << "  " << stc::colored("--no-coercion",        flag_col)   << "        disable all literal type coercion\n";
    std::cout << "  " << stc::colored("--fwd-fns",            flag_col)   << "            enable blind forwarding for non-backend-resolvable function calls (may lead to invalid code)\n";
    std::cout << "  " << stc::colored("--no-uniform-capture", flag_col)   << " don't capture Julia-resolvable globals as uniforms in the output\n";
    std::cout << '\n';
    
    std::cout << stc::colored("debugging:\n", title_col);
    std::cout << "  " << stc::colored("--dump-parsed",     flag_col)     << "        print the parsed version of the Julia AST\n";
    std::cout << "  " << stc::colored("--dump-sema",       flag_col)     << "          print the AST after semantic analysis\n";
    std::cout << "  " << stc::colored("--dump-lowered",    flag_col)     << "       print the lowered (SIR) representation\n";
    std::cout << "  " << stc::colored("--dump-scopes",     flag_col)     << "        dump scope tree info during semantic analysis\n";
    std::cout << "  " << stc::colored("--track-bindings",  flag_col)     << "     step-by-step reasoning for symbol binding resolution\n";
    std::cout << '\n';
    
    std::cout << stc::colored("formatting:\n", title_col);
    std::cout << "  " << stc::colored("--tabs",          flag_col)      << "               use tabs for indentation\n";
    std::cout << "  " << stc::colored("--spaces",        flag_col)      << "             use spaces for indentation (default)\n";
    std::cout << "  " << stc::colored("--cg-indent <n>", flag_col)      << "      set indentation width (default: 4)\n";
    std::cout << '\n';

    std::cout << stc::colored("errors and warnings:\n", title_col);
    std::cout << "  " << stc::colored("--err-dump none",    flag_col)   << "      disable AST dumping on errors (default)\n";
    std::cout << "  " << stc::colored("--err-dump partial", flag_col)   << "   dump the AST subtree that caused the given error\n";
    std::cout << "  " << stc::colored("--err-dump verbose", flag_col)   << "   dump the full AST at every error reported\n";
    std::cout << "  " << stc::colored("-Wfwd-fns",          flag_col)   << "            warn when a function call is forwarded (see --fwd-fns)\n";
    std::cout << "  " << stc::colored("-Wjl-query",         flag_col)   << "           warn when the Julia runtime was queried for function call resolution\n";
    // clang-format on
}

template <size_t N>
consteval auto n_chars_of(char sep) {
    std::array<char, N> arr{};

    for (size_t i = 0; i < N; i++)
        arr[i] = sep;

    return arr;
}

inline void print_version_info() {
    static constexpr auto val_col = ansi_codes::yellow;

    static constexpr std::string_view title   = "Shader Transpiler Core (STC)";
    static constexpr std::string_view stc_ver = stc::meta::version;

    static constexpr uint8_t padding_width = 2;
    static constexpr auto padding_buf      = n_chars_of<padding_width>(' ');
    static constexpr auto padding = std::string_view{padding_buf.data(), padding_buf.size()};

    static constexpr size_t header_width = title.size() + 2 * padding_width + stc_ver.size() + 2;

    static constexpr auto sep_line_buf = n_chars_of<header_width>('-');
    static constexpr auto sep_line     = std::string_view{sep_line_buf.data(), sep_line_buf.size()};

    std::cout << sep_line << '\n';
    std::cout << padding << title << stc::colored(" v", val_col) << stc::colored(stc_ver, val_col)
              << padding << '\n';
    std::cout << sep_line << "\n\n";

    // clang-format off
    std::cout << "target:         " << stc::colored(fmt::format("{}-{}", stc::meta::system_arch, stc::meta::system_name), val_col) << '\n';
    std::cout << "compiler:       " << stc::colored(fmt::format("{} {}", stc::meta::compiler_id, stc::meta::compiler_ver), val_col) << '\n';
    std::cout << "build type:     " << stc::colored(stc::meta::build_type, val_col) << '\n';
    std::cout << "build commit:   " << stc::colored(stc::meta::build_commit, val_col) << '\n';
    std::cout << "julia compat:   " << stc::colored(stc::meta::julia_ver, val_col) << '\n';
    std::cout << "build date:     " << stc::colored(stc::meta::build_date, val_col) << '\n';
    // clang-format on
}

} // namespace stc::cli
