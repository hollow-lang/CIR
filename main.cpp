#include <iostream>
#include "core/cir.h"

int main() {
    CIR vm;
    Program prog;
    
    Function test_fn;
    test_fn.ops.push_back(Op(OpType::Load, 0, 10));
    test_fn.ops.push_back(Op(OpType::Load, 1, 20));
    test_fn.ops.push_back(Op(OpType::Add, 0, 1));
    test_fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["main"] = std::move(test_fn);
    prog.state.cf = "main";
    
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    std::cout << "Result: " << vm.pop() << std::endl;
    
    return 0;
}