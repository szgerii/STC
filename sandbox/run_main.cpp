#include <iostream>
#include <ir/type_descriptors.h>
#include <ir/type_pool.h>
#include <ir/types.h>
#include <julia.h>
#include <temp.h>

int main() {
    std::cout << "Output of Lib's Temp::retOne is " << stc::Temp::retOne() << '\n';

    return 0;
}
