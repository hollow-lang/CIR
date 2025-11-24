// TODO: write a simple minimal runtime for the transpiled C since we cant use std because we dont have CIR context

#include <fstream>
#include <iostream>
#include <vector>
#include <iomanip>
// NOTE: No need for implementation we will link with .so
//#define CIR_IMPLEMENTATION
#include "core/cir.h"

int counter = 0;

std::string gc() {
    return std::to_string(counter++);
}

std::string wts(Word w) {
    switch (w.type) {
        case WordType::Integer:
            return std::to_string(w.as_int());
        case WordType::Float:
            return std::to_string(w.as_float());
        case WordType::Pointer:
            if (w.has_flag(WordFlag::String)) {
                return "\"" + std::string(static_cast<char *>(w.as_ptr())) + "\"";
            }
            return "(void*)" + std::to_string(reinterpret_cast<uintptr_t>(w.as_ptr()));
        case WordType::Boolean:
            return w.as_bool() ? "true" : "false";
        case WordType::Null:
            return "NULL";
        default:
            throw std::runtime_error("unknown word type");
    }
}


void transpile_op(const Op &op) {
    switch (op.type) {
        case OpType::Mov:
            if (op.args[0].has_flag(WordFlag::Register)) {
                std::cout << "vregs[" + std::to_string(op.args[1].as_int()) + "] = vregs[" +
                    std::to_string(op.args[0].as_int()) + "];" << std::endl;
            } else {
                std::cout << "vregs[" + std::to_string(op.args[1].as_int()) + "] = " +
                    wts(op.args[0]) + ";" << std::endl;
            }
            break;
        case OpType::IAdd:
            std::cout << "vregs[0] = vregs[" +
                std::to_string(op.args[0].as_int()) + "] + vregs[" +
                std::to_string(op.args[1].as_int()) + "];" << std::endl;
            break;
        case OpType::Call:
            std::cout << "functions[\"" + (std::string)(const char*)op.args[0].as_ptr() + "\"]();" << std::endl;
            break;
        case OpType::CallExtern:
            std::cout << "externs[\"" + (std::string)(const char*)op.args[0].as_ptr() + "\"]();" << std::endl;
            break;
        case OpType::Ret:
            std::cout << "return;" << std::endl;
            break;
        default: std::cout << "something" << std::endl;
    }
}

void transpile_function(std::string name, const Function &fn) {
    std::replace(name.begin(), name.end(), ':', '_');

    std::cout << "void " << name << "() {" << std::endl;

    for (const auto &op: fn.ops) {
        transpile_op(op);
    }

    std::cout << "}" << std::endl;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: disassembly <bytecode>" << std::endl;
        return 1;
    }

    CIR vm;

    std::ifstream f(argv[1], std::ios::binary);
    if (!f) {
        std::cerr << "Cannot open bytecode file: " << argv[1] << std::endl;
        return 1;
    }

    std::vector<uint8_t> bytecode(
        (std::istreambuf_iterator<char>(f)),
        std::istreambuf_iterator<char>()
    );

    vm.from_bytecode(bytecode);

    Program &prog = vm.get_program();
    for (const auto &[name, func]: prog.functions) {
        transpile_function(name, func);
    }

    return 0;
}