#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <iomanip>
// NOTE: No need for implementation we will link with .so
//#define CIR_IMPLEMENTATION
#include "core/cir.h"

#include "core/asm.h"

std::string op_type_to_string(OpType type, Assembler &assembler) {
    for (auto i: assembler.opcode_map) {
        if (i.second.type == type) return i.first;
    }
    return "UnknownOpType";
}

void disassemble_function(const std::string &name, const Function &fn, Assembler &assembler) {
    std::cout << "Function: " << name << std::endl;
    for (size_t i = 0; i < fn.ops.size(); i++) {
        const Op &op = fn.ops[i];
        std::cout << "  [" << i << "] " << op_type_to_string(op.type, assembler);

        for (size_t j = 0; j < Config::OpArgCount; j++) {
            const Word &arg = op.args[j];
            if (arg.type == WordType::Null && arg.flags == 0) continue;
            std::cout << " ";
            arg.print();
        }
        std::cout << std::endl;
    }

    if (!fn.locals.empty()) {
        std::cout << "  Locals:" << std::endl;
        for (const auto &[id, w]: fn.locals) {
            std::cout << "    [" << id << "] = ";
            w.print();
            std::cout << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: disassembly <bytecode>" << std::endl;
        return 1;
    }

    CIR vm;
    Assembler assembler;

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
        disassemble_function(name, func, assembler);
        std::cout << std::endl;
    }

    return 0;
}