// TODO: remove and introduce frontend
#include <iostream>
#include "core/cir.h"

int main() {
    CIR cir = CIR();

    // TODO: fix load from bytecode
    std::vector<Op> ops = {
        {OpType::Push, {10}},
        {OpType::Push, {10}},
    };

    Program p = Program();
    Function f = Function();
    f.ops = ops;

    p.functions["main"] = f;

    cir.load_program(p);
    cir.execute_program();

    return 0;
}