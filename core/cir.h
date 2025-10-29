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

#pragma once
#include <array>
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

class Op {
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
};

class Function {
    std::vector<Op> ops{};
    std::unordered_map<std::string, Word> locals{};
    Config::DI_TYPE co{}; // current operation
};

class Program {
public: // fuck this shit let them all be public
    std::unordered_map<std::string, Function> functions{};
    struct {
        std::string cf{}; // name of the current function
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
        // TODO: implement
    }

    void execute_function(const std::string& name) {
        // TODO: implement
    }

    void execute_program() {
        // TODO: implement
    }

    // NOTE: takes ownership! wanna get access to Program? use `get_program` it gives you a reference to it
    void load_program(Program p) { program = std::move(p); }
    // returns read-write reference to program
    Program& get_program() { return program; }

};
