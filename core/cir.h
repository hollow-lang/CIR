#pragma once
#include <array>
#include <cstdint>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>

#include "config.h"

using Word = uint64_t;

enum class OpType : uint8_t {
    Nop,
    Mov,
    Push,
    Pop,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    And,
    Or,
    Xor,
    Not,
    Shl,
    Shr,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    Jmp,
    Jz,
    Jnz,
    Call,
    Ret,
    Load,
    Store,
    Inc,
    Dec,
};

class Op {
public:
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
    
    constexpr Op() noexcept = default;
    constexpr Op(OpType t, Word a0 = 0, Word a1 = 0) noexcept : type(t), args{a0, a1} {}
};

class Function {
public:
    std::vector<Op> ops{};
    std::unordered_map<std::string, Word> locals{};
    Config::DI_TYPE co{};
};

class Program {
public:
    std::unordered_map<std::string, Function> functions{};
    struct {
        std::string cf{};
    } state;
};

class CIR {
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::vector<Word> stack{};
    size_t stack_size{};
    Program program;
    std::vector<Config::DI_TYPE> call_stack{};

public:
    CIR() { 
        stack.reserve(Config::STACK_SIZE);
        registers.fill(0);
    }

    inline Word pop() {
        if (stack_size == 0) throw std::runtime_error("stack underflow");
        return stack[--stack_size];
    }

    inline void push(Word value) {
        if (stack_size >= Config::STACK_SIZE) throw std::runtime_error("stack overflow");
        if (stack_size >= stack.size()) stack.resize(stack_size + 1);
        stack[stack_size++] = value;
    }

    inline void move(Word w, uint16_t i) {
        registers[i] = w;
    }

    inline Word& get(uint16_t i) { return registers[i]; }

    void execute_op(Function& fn, Op& op) {
        switch(op.type) {
            case OpType::Nop:
                break;
            case OpType::Mov:
                registers[op.args[0]] = registers[op.args[1]];
                break;
            case OpType::Push:
                push(registers[op.args[0]]);
                break;
            case OpType::Pop:
                registers[op.args[0]] = pop();
                break;
            case OpType::Add:
                registers[op.args[0]] += registers[op.args[1]];
                break;
            case OpType::Sub:
                registers[op.args[0]] -= registers[op.args[1]];
                break;
            case OpType::Mul:
                registers[op.args[0]] *= registers[op.args[1]];
                break;
            case OpType::Div:
                if (registers[op.args[1]] == 0) throw std::runtime_error("division by zero");
                registers[op.args[0]] /= registers[op.args[1]];
                break;
            case OpType::Mod:
                if (registers[op.args[1]] == 0) throw std::runtime_error("modulo by zero");
                registers[op.args[0]] %= registers[op.args[1]];
                break;
            case OpType::And:
                registers[op.args[0]] &= registers[op.args[1]];
                break;
            case OpType::Or:
                registers[op.args[0]] |= registers[op.args[1]];
                break;
            case OpType::Xor:
                registers[op.args[0]] ^= registers[op.args[1]];
                break;
            case OpType::Not:
                registers[op.args[0]] = ~registers[op.args[0]];
                break;
            case OpType::Shl:
                if (registers[op.args[1]] < 64) {
                    registers[op.args[0]] <<= registers[op.args[1]];
                }
                break;
            case OpType::Shr:
                if (registers[op.args[1]] < 64) {
                    registers[op.args[0]] >>= registers[op.args[1]];
                }
                break;
            case OpType::Eq:
                registers[op.args[0]] = (registers[op.args[0]] == registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Ne:
                registers[op.args[0]] = (registers[op.args[0]] != registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Lt:
                registers[op.args[0]] = (registers[op.args[0]] < registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Le:
                registers[op.args[0]] = (registers[op.args[0]] <= registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Gt:
                registers[op.args[0]] = (registers[op.args[0]] > registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Ge:
                registers[op.args[0]] = (registers[op.args[0]] >= registers[op.args[1]]) ? 1 : 0;
                break;
            case OpType::Jmp:
                fn.co = op.args[0];
                return;
            case OpType::Jz:
                if (registers[op.args[0]] == 0) {
                    fn.co = op.args[1];
                    return;
                }
                break;
            case OpType::Jnz:
                if (registers[op.args[0]] != 0) {
                    fn.co = op.args[1];
                    return;
                }
                break;
            case OpType::Call:
                call_stack.push_back(fn.co + 1);
                fn.co = op.args[0];
                return;
            case OpType::Ret:
                if (!call_stack.empty()) {
                    fn.co = call_stack.back();
                    call_stack.pop_back();
                    return;
                }
                break;
            case OpType::Load:
                registers[op.args[0]] = op.args[1];
                break;
            case OpType::Store:
                fn.locals[std::to_string(op.args[0])] = registers[op.args[1]];
                break;
            case OpType::Inc:
                registers[op.args[0]]++;
                break;
            case OpType::Dec:
                registers[op.args[0]]--;
                break;
        }
        fn.co++;
    }

    void execute_function(const std::string& name) {
        auto it = program.functions.find(name);
        if (it == program.functions.end()) throw std::runtime_error("function not found");
        Function& fn = it->second;
        fn.co = 0;
        while (fn.co < fn.ops.size()) {
            execute_op(fn, fn.ops[fn.co]);
        }
    }

    void execute_program() {
        if (program.state.cf.empty() && !program.functions.empty()) {
            program.state.cf = program.functions.begin()->first;
        }
        if (!program.state.cf.empty()) {
            execute_function(program.state.cf);
        }
    }

    void load_program(Program p) { program = std::move(p); }
    Program& get_program() { return program; }
};
