#include <iostream>
#include <cassert>
#include "core/cir.h"

void test_arithmetic() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 15));
    fn.ops.push_back(Op(OpType::Load, 1, 5));
    fn.ops.push_back(Op(OpType::Add, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 20);
    std::cout << "Arithmetic test passed" << std::endl;
}

void test_subtraction() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 30));
    fn.ops.push_back(Op(OpType::Load, 1, 10));
    fn.ops.push_back(Op(OpType::Sub, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 20);
    std::cout << "Subtraction test passed" << std::endl;
}

void test_multiplication() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 5));
    fn.ops.push_back(Op(OpType::Load, 1, 6));
    fn.ops.push_back(Op(OpType::Mul, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 30);
    std::cout << "Multiplication test passed" << std::endl;
}

void test_division() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 60));
    fn.ops.push_back(Op(OpType::Load, 1, 3));
    fn.ops.push_back(Op(OpType::Div, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 20);
    std::cout << "Division test passed" << std::endl;
}

void test_bitwise() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 0xFF));
    fn.ops.push_back(Op(OpType::Load, 1, 0x0F));
    fn.ops.push_back(Op(OpType::And, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 0x0F);
    std::cout << "Bitwise test passed" << std::endl;
}

void test_comparison() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 10));
    fn.ops.push_back(Op(OpType::Load, 1, 20));
    fn.ops.push_back(Op(OpType::Lt, 0, 1));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 1);
    std::cout << "Comparison test passed" << std::endl;
}

void test_inc_dec() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 10));
    fn.ops.push_back(Op(OpType::Inc, 0));
    fn.ops.push_back(Op(OpType::Inc, 0));
    fn.ops.push_back(Op(OpType::Dec, 0));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 11);
    std::cout << "Inc/Dec test passed" << std::endl;
}

void test_jump() {
    CIR vm;
    Program prog;
    Function fn;
    
    fn.ops.push_back(Op(OpType::Load, 0, 5));
    fn.ops.push_back(Op(OpType::Jmp, 3));
    fn.ops.push_back(Op(OpType::Load, 0, 99));
    fn.ops.push_back(Op(OpType::Push, 0));
    
    prog.functions["test"] = std::move(fn);
    prog.state.cf = "test";
    vm.load_program(std::move(prog));
    vm.execute_program();
    
    assert(vm.pop() == 5);
    std::cout << "Jump test passed" << std::endl;
}

int main() {
    test_arithmetic();
    test_subtraction();
    test_multiplication();
    test_division();
    test_bitwise();
    test_comparison();
    test_inc_dec();
    test_jump();
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
