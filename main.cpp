// TODO: remove and introduce frontend
#include <iostream>
#include <fstream>
#include "core/cir.h"

int main() {
    CIR cir = CIR();

    // TODO: compiler
    //std::ifstream file("program.bin", std::ios::binary);
    //if (!file.is_open()) {
    //    std::cerr << "Failed to open program.bin" << std::endl;
    //    return 1;
    //}
    //
    //std::vector<uint8_t> bytecode((std::istreambuf_iterator<char>(file)),
    //                              std::istreambuf_iterator<char>());
    //file.close();
    //
    //cir.from_bytecode(bytecode);

    Program program;
    Function main;
    Function abc;
    Op main_ops[] = {
        {OpType::Call, {Word::from_string_owned("abc")}},
        {OpType::Nop}
    };
    main.ops.append_range(main_ops);

    Op abc_ops[] = {
        {OpType::Mov, {Word::from_int(10), Word::from_int(0)}},
        {OpType::Mov, {Word::from_int(10), Word::from_int(1)}},
        {OpType::Add, {Word::from_int(0), Word::from_int(1)}},
        {OpType::PushReg, {Word::from_int(0)}},
        {OpType::Ret, {}}
    };
    abc.ops.append_range(abc_ops);

    program.functions["main"] = main;
    program.functions["abc"] = abc;
    cir.load_program(program);

    cir.execute_program();

    for (auto& si : cir.get_stack() ) {
        switch (si.type) {
            case WordType::Integer: {
                std::cout << si.as_int() << " ";
            } break;
            case WordType::Float: {
                std::cout << si.as_float() << " ";
            } break;
            case WordType::Pointer: {
                std::cout << si.as_ptr() << " ";
            } break;
            case WordType::Boolean: {
                std::cout << si.as_bool() << " ";
            }
            default: assert(0 && "unreachable");
        }
    }

    std::vector<uint8_t> out_bytecode = cir.to_bytecode();
    std::ofstream out_file("program.bin", std::ios::binary);
    out_file.write(reinterpret_cast<char*>(out_bytecode.data()), static_cast<long>(out_bytecode.size()));

    return 0;
}