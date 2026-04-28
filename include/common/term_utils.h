#pragma once

#include <iostream>
#include <string_view>

#ifdef _WIN32
    #include <io.h>
    #include <windows.h>
#else
    #include <unistd.h>
#endif

namespace ansi_codes {

inline constexpr std::string_view black  = "\033[30m";
inline constexpr std::string_view red    = "\033[31m";
inline constexpr std::string_view green  = "\033[32m";
inline constexpr std::string_view yellow = "\033[33m";
inline constexpr std::string_view blue   = "\033[34m";
inline constexpr std::string_view purple = "\033[35m";
inline constexpr std::string_view cyan   = "\033[36m";
inline constexpr std::string_view white  = "\033[37m";
inline constexpr std::string_view reset  = "\033[0m";

}; // namespace ansi_codes

namespace stc {

class TerminalInfo {
public:
    static bool supports_color();
};

// wrapper to avoid string allocation
struct ColoredText {
    std::string_view text;
    std::string_view color_code;

    friend std::ostream& operator<<(std::ostream& out, const ColoredText& ct) {
        if (TerminalInfo::supports_color())
            out << ct.color_code;

        out << ct.text;

        if (TerminalInfo::supports_color())
            out << ansi_codes::reset;

        return out;
    }
};

inline constexpr ColoredText colored(std::string_view text, std::string_view color) {
    return {text, color};
}

} // namespace stc
