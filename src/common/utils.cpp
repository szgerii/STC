#include <iostream>

#include "common/term_utils.h"
#include "common/utils.h"

namespace stc {

void report(std::string_view msg, std::ostream& out) {
    out << msg << '\n';
}

void error(std::string_view msg, std::ostream& out) {
    out << colored("[Error] ", ansi_codes::red);
    report(msg, out);
}

void warning(std::string_view msg, std::ostream& out) {
    out << colored("[Warning] ", ansi_codes::yellow);
    report(msg, out);
}

void internal_error(std::string_view msg, std::ostream& out) {
    out << colored("[Internal Error] ", ansi_codes::red);
    report(msg, out);
}

} // namespace stc