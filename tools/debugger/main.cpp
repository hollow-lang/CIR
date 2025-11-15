#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <set>

#include "core/cir.h"
#include "core/std.h"
#include "core/asm.h"

std::string op_type_to_string(OpType type, Assembler &assembler) {
    for (const auto &i: assembler.opcode_map) {
        if (i.second.type == type) return i.first;
    }
    return "UnknownOpType";
}

class Debugger {
private:
    CIR &vm;
    Program &program;
    Assembler &assembler;
    std::set<size_t> breakpoints;
    bool step_mode = true;

    void print_registers() const {
        std::cout << "\n=== Registers ===" << std::endl;
        for (int i = 0; i < 8; i++) {
            std::cout << "r" << i << ": ";
            vm.getr(i).print();
            std::cout << std::endl;
        }
    }

    void print_stack() const {
        std::cout << "\n=== Stack (top 5) ===" << std::endl;
        auto &stack = vm.get_stack();
        size_t start = stack.size() > 5 ? stack.size() - 5 : 0;
        for (size_t i = start; i < stack.size(); i++) {
            std::cout << "[" << i << "]: ";
            stack[i].print();
            std::cout << std::endl;
        }
        if (stack.empty()) {
            std::cout << "(empty)" << std::endl;
        }
    }

    void print_current_instruction(Function &fn) const {
        if (fn.co >= fn.ops.size()) {
            std::cout << "End of function" << std::endl;
            return;
        }

        Op &op = fn.ops[fn.co];
        std::cout << "\n[" << fn.co << "] " << op_type_to_string(op.type, assembler);

        for (size_t i = 0; i < Config::OpArgCount; i++) {
            if (op.args[i].type != WordType::Null) {
                std::cout << " ";
                op.args[i].print();
            }
        }
        std::cout << std::endl;
    }

    static void print_help() {
        std::cout << "\n=== Debugger Commands ===" << std::endl;
        std::cout << "n/next    - Execute next instruction" << std::endl;
        std::cout << "c/cont    - Continue until breakpoint" << std::endl;
        std::cout << "r/regs    - Show registers" << std::endl;
        std::cout << "s/stack   - Show stack" << std::endl;
        std::cout << "b <addr>  - Set breakpoint at address" << std::endl;
        std::cout << "d <addr>  - Delete breakpoint" << std::endl;
        std::cout << "l/list    - List breakpoints" << std::endl;
        std::cout << "h/help    - Show this help" << std::endl;
        std::cout << "q/quit    - Quit debugger" << std::endl;
    }

    static std::string get_command() {
        std::cout << "\n> ";
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

public:
    Debugger(CIR &vm, Program &prog, Assembler &asm_)
        : vm(vm), program(prog), assembler(asm_) {
    }

    void debug_function(const std::string &name) {
        program.state.cf = name;
        program.state.running = true;

        if (!program.functions.contains(name)) {
            throw std::runtime_error("Function not found: " + name);
        }
        program.functions[name].co = 0;

        std::cout << "\n=== Debugging function: " << name << " ===" << std::endl;
        print_help();

        while (program.state.running) {
            Function &fn = program.functions[program.state.cf];

            if (fn.co >= fn.ops.size()) {
                if (program.state.call_stack.empty()) {
                    program.state.running = false;
                    std::cout << "\nProgram ended." << std::endl;
                    break;
                }

                CallFrame cf = program.state.call_stack.back();
                program.state.call_stack.pop_back();
                program.state.cf = cf.name;
                program.functions[program.state.cf].co = cf.co;
                continue;
            }

            if (breakpoints.count(fn.co) && !step_mode) {
                std::cout << "\nBreakpoint hit at address " << fn.co << std::endl;
                step_mode = true;
            }

            if (step_mode) {
                print_current_instruction(fn);

                std::string cmd = get_command();
                std::istringstream iss(cmd);
                std::string action;
                iss >> action;

                if (action == "n" || action == "next" || action.empty()) {
                } else if (action == "c" || action == "cont") {
                    step_mode = false;
                } else if (action == "r" || action == "regs") {
                    print_registers();
                    continue;
                } else if (action == "s" || action == "stack") {
                    print_stack();
                    continue;
                } else if (action == "b") {
                    size_t addr;
                    if (iss >> addr) {
                        breakpoints.insert(addr);
                        std::cout << "Breakpoint set at " << addr << std::endl;
                    } else {
                        std::cout << "Usage: b <address>" << std::endl;
                    }
                    continue;
                } else if (action == "d") {
                    size_t addr;
                    if (iss >> addr) {
                        breakpoints.erase(addr);
                        std::cout << "Breakpoint removed at " << addr << std::endl;
                    } else {
                        std::cout << "Usage: d <address>" << std::endl;
                    }
                    continue;
                } else if (action == "l" || action == "list") {
                    std::cout << "Breakpoints: ";
                    for (auto bp: breakpoints) {
                        std::cout << bp << " ";
                    }
                    std::cout << (breakpoints.empty() ? "(none)" : "") << std::endl;
                    continue;
                } else if (action == "h" || action == "help") {
                    print_help();
                    continue;
                } else if (action == "q" || action == "quit") {
                    std::cout << "Exiting debugger." << std::endl;
                    return;
                } else {
                    std::cout << "Unknown command. Type 'h' for help." << std::endl;
                    continue;
                }
            }

            vm.execute_op(fn, fn.ops[fn.co]);
            fn.co++;
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: debugger <bytecode>" << std::endl;
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
    cir_std::init_std(vm);

    Program &prog = vm.get_program();

    Debugger debugger(vm, prog, assembler);
    debugger.debug_function("main");

    return 0;
}
