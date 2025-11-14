#ifndef STD_H
#define STD_H

#include <iostream>

#include "cir.h"

namespace cir_std {
    void print(CIR &cir) {
        cir.getr(0).print();
        std::cout << std::endl;
    }

    void init_std(CIR &cir) {
        cir.set_extern_fn("std.print", print);
    }
}


#endif //STD_H
