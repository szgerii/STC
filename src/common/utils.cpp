#include <iostream>

#include "common/utils.h"

namespace stc {

std::nullptr_t report(SrcLocation location, std::string_view msg, bool is_warning) {
    std::cerr << '[' << location.file->path << ':' << location.line << ':' << location.col << "] "
              << (is_warning ? "warning: " : "error: ") << msg << '\n';

    // TODO: print code snippet

    return nullptr;
}

std::nullptr_t error(SrcLocation location, std::string_view msg) {
    return report(location, msg, false);
}

std::nullptr_t warning(SrcLocation location, std::string_view msg) {
    return report(location, msg, true);
}

} // namespace stc