#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>

#include "config.h"

enum class WordType : uint8_t {
    Integer,
    Float,
    Pointer,
    Boolean,
    Null
};

struct Word {
    WordType type{WordType::Null};

    union {
        int64_t i;
        double f;
        uint64_t p;
        bool b;
    } data{};

    Word() : type(WordType::Null) { data.i = 0; }

    static Word from_int(int64_t val) {
        Word w;
        w.type = WordType::Integer;
        w.data.i = val;
        return w;
    }

    static Word from_float(double val) {
        Word w;
        w.type = WordType::Float;
        w.data.f = val;
        return w;
    }

    static Word from_ptr(uint64_t val) {
        Word w;
        w.type = WordType::Pointer;
        w.data.p = val;
        return w;
    }

    static Word from_bool(bool val) {
        Word w;
        w.type = WordType::Boolean;
        w.data.b = val;
        return w;
    }

    int64_t as_int() const { return data.i; }
    double as_float() const { return data.f; }
    uint64_t as_ptr() const { return data.p; }
    bool as_bool() const { return data.b; }
};

enum class OpType : uint8_t {
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
    Cmp,
    Jmp,
    Je,
    Jne,
    Jg,
    Jl,
    Jge,
    Jle,
    Call,
    Ret,
    Load,
    Store,
    Halt,
    Nop,
    Inc,
    Dec,
    Neg,
    FAdd,
    FSub,
    FMul,
    FDiv,
    FCmp,
    I2F,
    F2I
};

struct Op {
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
};

struct Function {
    std::vector<Op> ops{};
    std::unordered_map<std::string, Word> locals{};
    Config::DI_TYPE co{};
};

class Program {
public:
    std::unordered_map<std::string, Function> functions{};

    struct {
        std::string cf{};
        bool running = true;
    } state;
};

class CIR {
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::vector<Word> stack{};
    std::vector<uint64_t> call_stack{};
    bool cmp_flag{false};
    Program program;

public:
    Word pop() {
        Word top = stack.back();
        stack.pop_back();
        return top;
    }

    void push(const Word &value) {
        stack.push_back(value);
    }

    void move(const Word &w, uint16_t i) {
        registers[i] = w;
    }

    // get register
    Word &getr(uint16_t i) {
        return registers[i];
    }

    // get stack
    Word &gets() {
        return stack.emplace_back();
    }

    void execute_op(Function &fn, Op op) {
        switch (op.type) {
            // (value, target)
            case OpType::Mov: {
                move(op.args[0], op.args[1].as_int());
            }
            break;

            // (value)
            case OpType::Push: {
                push(op.args[0]);
            }
            break;

            // (target)
            case OpType::Pop: {
                Word &r = getr(op.args[0].as_int());
                r = pop();
            }
            break;

            // (r1, r2)
            case OpType::Add: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() + b.as_int());
            }
            break;

            // (r1, r2) = r1.val - r2.val
            case OpType::Sub: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() - b.as_int());
            }
            break;

            // (r1 * r2)
            case OpType::Mul: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() * b.as_int());
            }
            break;

            // (r1 / r2)
            case OpType::Div: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                if (b.as_int() == 0) {
                    throw std::runtime_error("Division by zero");
                }
                dest = Word::from_int(a.as_int() / b.as_int());
            }
            break;

            // (r1 % r2)
            case OpType::Mod: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                if (b.as_int() == 0) {
                    throw std::runtime_error("Modulo by zero");
                }
                dest = Word::from_int(a.as_int() % b.as_int());
            }
            break;

            // (r1 & r2)
            case OpType::And: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() & b.as_int());
            }
            break;

            // (r1 | r2)
            case OpType::Or: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() | b.as_int());
            }
            break;

            // (r1 ^ r2)
            case OpType::Xor: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() ^ b.as_int());
            }
            break;

            // (~r1)
            case OpType::Not: {
                Word &a = getr(op.args[0].as_int());
                Word &dest = gets();
                dest = Word::from_int(~a.as_int());
            }
            break;

            // (r1 << r2)
            case OpType::Shl: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() << b.as_int());
            }
            break;

            // (r1 >> r2)
            case OpType::Shr: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_int(a.as_int() >> b.as_int());
            }
            break;

            // (r1 == r2)
            case OpType::Cmp: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                cmp_flag = (a.as_int() == b.as_int());
            }
            break;

            // jump_addr(r1)
            case OpType::Jmp: {
                fn.co = op.args[0].as_int();
            }
            break;

            // jump_addr_if_equal(r1)
            case OpType::Je: {
                if (cmp_flag) {
                    fn.co = op.args[0].as_int();
                }
            }
            break;

            // jump_addr_if_not_equal(r1)
            case OpType::Jne: {
                if (!cmp_flag) {
                    fn.co = op.args[0].as_int();
                }
            }
            break;

            // (r1++)
            case OpType::Inc: {
                Word &r = getr(op.args[0].as_int());
                r = Word::from_int(r.as_int() + 1);
            }
            break;

            // (r1--)
            case OpType::Dec: {
                Word &r = getr(op.args[0].as_int());
                r = Word::from_int(r.as_int() - 1);
            }
            break;

            // (-r1)
            case OpType::Neg: {
                Word &a = getr(op.args[0].as_int());
                Word &dest = gets();
                dest = Word::from_int(-a.as_int());
            }
            break;

            // float(r1 + r2)
            case OpType::FAdd: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_float(a.as_float() + b.as_float());
            }
            break;

            // float(r1 - r2)
            case OpType::FSub: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_float(a.as_float() - b.as_float());
            }
            break;

            // float(r1 * r2)
            case OpType::FMul: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_float(a.as_float() * b.as_float());
            }
            break;

            // float(r1 * r2)
            case OpType::FDiv: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                Word &dest = gets();
                dest = Word::from_float(a.as_float() / b.as_float());
            }
            break;

            // cast_f(r1) -> r1
            case OpType::I2F: {
                Word &a = getr(op.args[0].as_int());
                a = Word::from_float(static_cast<double>(a.as_int()));
            }
            break;

            // cast_i(r1) -> r1
            case OpType::F2I: {
                Word &a = getr(op.args[0].as_int());
                a = Word::from_int(static_cast<int64_t>(a.as_float()));
            }
            break;

            case OpType::Halt: {
                program.state.running = false;
            }
            break;

            case OpType::Nop: break;

            default: assert(0 && "wtf, this dont should happen.");
        }
    }

    void execute_function(const std::string &name) {
        if (!program.functions.contains(name)) {
            throw std::runtime_error("Function not found: " + name);
        }
        Function &f = program.functions[name];

        f.co = 0;
        while (f.co < f.ops.size() && program.state.running) {
            execute_op(f, f.ops[f.co]);
            f.co++;
        }
    }

    void execute_program() {
        execute_function("main");
    }

    std::vector<uint8_t> to_bytecode() {
        std::vector<uint8_t> bytes;

        uint32_t func_count = program.functions.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&func_count),
                     reinterpret_cast<uint8_t *>(&func_count) + sizeof(func_count));

        for (const auto &[name, func]: program.functions) {
            uint32_t name_len = name.size();
            bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&name_len),
                         reinterpret_cast<uint8_t *>(&name_len) + sizeof(name_len));
            bytes.insert(bytes.end(), name.begin(), name.end());

            uint32_t op_count = func.ops.size();
            bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&op_count),
                         reinterpret_cast<uint8_t *>(&op_count) + sizeof(op_count));

            for (const auto &op: func.ops) {
                bytes.push_back(static_cast<uint8_t>(op.type));

                for (size_t i = 0; i < Config::OpArgCount; i++) {
                    bytes.push_back(static_cast<uint8_t>(op.args[i].type));
                    bytes.insert(bytes.end(),
                                 reinterpret_cast<const uint8_t *>(&op.args[i].data),
                                 reinterpret_cast<const uint8_t *>(&op.args[i].data) + sizeof(op.args[i].data));
                }
            }

            uint32_t local_count = func.locals.size();
            bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&local_count),
                         reinterpret_cast<uint8_t *>(&local_count) + sizeof(local_count));

            for (const auto &[local_name, local_val]: func.locals) {
                uint32_t local_name_len = local_name.size();
                bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&local_name_len),
                             reinterpret_cast<uint8_t *>(&local_name_len) + sizeof(local_name_len));
                bytes.insert(bytes.end(), local_name.begin(), local_name.end());

                bytes.push_back(static_cast<uint8_t>(local_val.type));
                bytes.insert(bytes.end(),
                             reinterpret_cast<const uint8_t *>(&local_val.data),
                             reinterpret_cast<const uint8_t *>(&local_val.data) + sizeof(local_val.data));
            }
        }

        return bytes;
    }

    void from_bytecode(const std::vector<uint8_t> &bytes) {
        size_t offset = 0;
        program = Program{};

        if (bytes.size() < sizeof(uint32_t)) {
            throw std::runtime_error("Bytecode too short: cannot read function count");
        }

        uint32_t func_count;
        std::memcpy(&func_count, &bytes[offset], sizeof(func_count));
        offset += sizeof(func_count);

        for (uint32_t f = 0; f < func_count; f++) {
            if (offset + sizeof(uint32_t) > bytes.size()) {
                throw std::runtime_error("Bytecode truncated: cannot read function name length");
            }

            uint32_t name_len;
            std::memcpy(&name_len, &bytes[offset], sizeof(name_len));
            offset += sizeof(name_len);

            if (offset + name_len > bytes.size()) {
                throw std::runtime_error("Bytecode truncated: cannot read function name");
            }

            std::string func_name(bytes.begin() + offset, bytes.begin() + offset + name_len);
            offset += name_len;

            Function func;

            if (offset + sizeof(uint32_t) > bytes.size()) {
                throw std::runtime_error("Bytecode truncated: cannot read op count");
            }

            uint32_t op_count;
            std::memcpy(&op_count, &bytes[offset], sizeof(op_count));
            offset += sizeof(op_count);

            for (uint32_t o = 0; o < op_count; o++) {
                if (offset + 1 > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read op type");
                }

                Op op;
                op.type = static_cast<OpType>(bytes[offset++]);

                for (size_t i = 0; i < Config::OpArgCount; i++) {
                    if (offset + 1 + sizeof(op.args[i].data) > bytes.size()) {
                        throw std::runtime_error("Bytecode truncated: cannot read op argument");
                    }

                    op.args[i].type = static_cast<WordType>(bytes[offset++]);
                    std::memcpy(&op.args[i].data, &bytes[offset], sizeof(op.args[i].data));
                    offset += sizeof(op.args[i].data);
                }

                func.ops.push_back(op);
            }

            if (offset + sizeof(uint32_t) > bytes.size()) {
                throw std::runtime_error("Bytecode truncated: cannot read local count");
            }

            uint32_t local_count;
            std::memcpy(&local_count, &bytes[offset], sizeof(local_count));
            offset += sizeof(local_count);

            for (uint32_t l = 0; l < local_count; l++) {
                if (offset + sizeof(uint32_t) > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read local name length");
                }

                uint32_t local_name_len;
                std::memcpy(&local_name_len, &bytes[offset], sizeof(local_name_len));
                offset += sizeof(local_name_len);

                if (offset + local_name_len > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read local name");
                }

                std::string local_name(bytes.begin() + offset, bytes.begin() + offset + local_name_len);
                offset += local_name_len;

                if (offset + 1 + sizeof(Word::data) > bytes.size()) {
                    throw std::runtime_error(
                        "Bytecode truncated: cannot read local value: need " +
                        std::to_string(offset + 1 + sizeof(Word::data)) +
                        " bytes but only have " + std::to_string(bytes.size())
                    );
                }

                Word local_val;
                local_val.type = static_cast<WordType>(bytes[offset++]);
                std::memcpy(&local_val.data, &bytes[offset], sizeof(local_val.data));
                offset += sizeof(local_val.data);

                func.locals[local_name] = local_val;
            }

            program.functions[func_name] = func;
        }
    }

    void load_program(Program p) {
        program = std::move(p);
    }

    Program &get_program() {
        return program;
    }

    std::vector<Word> &get_stack() {
        return stack;
    }
};
