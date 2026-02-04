#pragma once

#ifdef _WIN32
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT
#endif

namespace stc {

class Test {
public:
    LIB_EXPORT static void sayHi();
};

} // namespace stc