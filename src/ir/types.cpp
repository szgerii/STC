#include "ir/types.h"

namespace stc::ir {

bool Type::has_qualifiers(Qualifier filter) const {
    return has_any(qualifiers, filter);
}

} // namespace stc::ir