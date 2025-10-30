/*
CIR - Cross-platform Runtime/VM
A register-based virtual machine (VM) that executes Programs.

Program:
    - Contains all functions and its own state (currently only the current function but possibly will grow in future).

Function:
    - Contains a sequence of operations (Ops) and its own state (locals and current operation index).

Op:
    - A single operation executed within a Function, containing its type and arguments.
*/

// TODO: complete design something is 100% not ready
#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "config.h"

// TODO: change (temporary solution)
using Word = uint64_t;

enum class OpType : uint8_t {
    Mov,
    Push,
    Pop,
    // TODO: more instructions, locals
};

struct Op {
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
};

struct Function {
    std::vector<Op> ops{};
    std::unordered_map<std::string, Word> locals{};
    Config::DI_TYPE co{}; // current operation
};

class Program {
public: // fuck this shit let them, all be public
    std::unordered_map<std::string, Function> functions{};
    struct {
        std::string cf{}; // name of the current function
        bool running = true;
    } state;
};

class CIR {
    // TODO: decide should be moved to Program or not
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::stack<Word> stack{};

    // NOTE: owned memory
    Program program;

public:
    Word pop() {
        Word top = stack.top(); // Dereferences reference gets copy
        stack.pop();
        return top; // returns copy
    }

    void push(Word& value) { stack.push(value); }

    void move(Word w, uint16_t i) {
        registers[i] = w; // assignes registers + (i * sizeof(Word)) the value w
    }

    // returns a read-write reference to a register
    Word& get(uint16_t i) { return registers[i]; }

    void execute_op(Function& fn, Op op) {
        // TODO: change atleast to jump table
        switch (op.type) {
            case OpType::Mov: {
                assert(0 && "TODO implement");
            } break;
            case OpType::Push: {
                assert(0 && "TODO implement");
            } break;
                assert(0 && "TODO implement");
            case OpType::Pop: {
                assert(0 && "TODO implement");
            } break;

            default: {
                assert(0 && "Invalid Op");
            }
        }

    }

    void execute_function(const std::string& name) {
        if (!program.functions.contains(name)) {
            assert(0 && "Function not found");
        }
        Function f = program.functions[name];

        for (auto& op : f.ops) {
            if (!program.state.running) {
                break;
            }

            execute_op(f, op);
        }
    }

    // Executes execute_function using the default entry point "main"
    void execute_program() {
        execute_function("main");
    }

    std::vector<uint8_t> to_bytecode() {
        assert(0 && "TODO");
    }

    void from_bytecode(std::vector<uint8_t> bytes) {
        assert(0 && "TODO");
    }

    // NOTE: takes ownership! wanna get access to Program? use `get_program` it gives you a reference to it
    void load_program(Program p) { program = std::move(p); }
    // returns read-write reference to program
    Program& get_program() { return program; }
};
