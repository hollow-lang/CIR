#include <iostream>

#include "../../core/cir.h"

void example_fn(CIR& vm) {
    std::cout << "Hello, world!" << std::endl;
}

extern "C" void cir_init_lib(CIR& vm) {
    vm.set_extern_fn("example.example_fn", example_fn);
}