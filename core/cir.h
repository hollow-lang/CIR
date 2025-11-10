// TODO: implement dl loading from files
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

class CIR;

using CIR_ExternFn = void (*)(CIR& vm);

enum class WordType : uint8_t {
    Integer,
    Float,
    Pointer,
    Boolean,
    Null
};

enum class WordFlag : uint8_t {
    None = 0,
    String = 1 << 1,
    OwnsMemory = 1 << 2,
};

struct Word {
    WordType type{WordType::Null};
    uint8_t flags = 0;

    union {
        int64_t i;
        double f;
        void *p;
        bool b;
    } data{};

    Word() { data.i = 0; }

    Word(const Word &other) : type(other.type), flags(other.flags) {
        if (other.has_flag(WordFlag::OwnsMemory) && other.has_flag(WordFlag::String) && other.data.p != nullptr) {
            const char *src = static_cast<const char *>(other.data.p);
            size_t len = std::strlen(src);
            char *str_copy = new char[len + 1];
            std::strcpy(str_copy, src);
            data.p = str_copy;
        } else {
            data = other.data;
        }
    }

    Word &operator=(const Word &other) {
        if (this != &other) {
            if (has_flag(WordFlag::OwnsMemory) && has_flag(WordFlag::String) && data.p != nullptr) {
                delete[] static_cast<char *>(data.p);
            }

            type = other.type;
            flags = other.flags;

            if (other.has_flag(WordFlag::OwnsMemory) && other.has_flag(WordFlag::String) && other.data.p != nullptr) {
                const char *src = static_cast<const char *>(other.data.p);
                size_t len = std::strlen(src);
                char *str_copy = new char[len + 1];
                std::strcpy(str_copy, src);
                data.p = str_copy;
            } else {
                data = other.data;
            }
        }
        return *this;
    }

    Word(Word &&other) noexcept : type(other.type), flags(other.flags), data(other.data) {
        other.flags = 0;
        other.data.p = nullptr;
    }

    Word &operator=(Word &&other) noexcept {
        if (this != &other) {
            if (has_flag(WordFlag::OwnsMemory) && has_flag(WordFlag::String) && data.p != nullptr) {
                delete[] static_cast<char *>(data.p);
            }

            type = other.type;
            flags = other.flags;
            data = other.data;

            other.flags = 0;
            other.data.p = nullptr;
        }
        return *this;
    }

    ~Word() {
        if (has_flag(WordFlag::OwnsMemory) && has_flag(WordFlag::String) && data.p != nullptr) {
            delete[] static_cast<char *>(data.p);
        }
    }

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

    static Word from_ptr(void *val) {
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

    static Word from_string(const std::string &val) {
        Word w;
        w.type = WordType::Pointer;
        w.set_flag(WordFlag::String);
        w.data.p = const_cast<char *>(val.c_str());
        return w;
    }

    static Word from_string_owned(const std::string &val) {
        Word w;
        w.type = WordType::Pointer;
        w.set_flag(WordFlag::String);
        w.set_flag(WordFlag::OwnsMemory);

        char *str_copy = new char[val.size() + 1];
        std::strcpy(str_copy, val.c_str());
        w.data.p = str_copy;
        return w;
    }

    static Word from_null() {
        Word w;
        w.type = WordType::Null;
        return w;
    }

    void set_flag(WordFlag flag) {
        flags |= static_cast<uint8_t>(flag);
    }

    [[nodiscard]] bool has_flag(WordFlag flag) const {
        return (flags & static_cast<uint8_t>(flag)) != 0;
    }

    [[nodiscard]] int64_t as_int() const { return data.i; }
    [[nodiscard]] double as_float() const { return data.f; }
    [[nodiscard]] void *as_ptr() const { return data.p; }
    [[nodiscard]] bool as_bool() const { return data.b; }

    constexpr static void expect(Word& w, WordType type, const char *msg) {
        if (w.type != type) {
            throw std::runtime_error("Expected " + std::to_string(static_cast<int>(type)) + " but got " +
                                     std::to_string(static_cast<int>(w.type)) + ": " + msg);
        }
    }
};

enum class OpType : uint8_t {
    Mov,
    Push, // Push value
    PushReg, // push register
    Pop,
    IAdd,
    ISub,
    IMul,
    IDiv,
    IMod,
    And,
    Or,
    Xor,
    Not,
    Shl,
    Shr,
    ICmp,
    Jmp,
    Je,
    Jne,
    Jg, // TODO: implement
    Jl, // TODO: implement
    Jge, // TODO: implement
    Jle, // TODO: implement
    Call,
    CallExtern,
    Ret,
    Load, // TODO: Implement
    Store, // TODO: Implement
    Halt,
    Nop,
    Inc,
    Dec,
    Neg,
    FAdd,
    FSub,
    FMul,
    FDiv,
    FCmp, // TODO: implement
    Cast,
    LocalGet,
    LocalSet,
};

struct Op {
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
};

struct Function {
    std::vector<Op> ops{};
    std::unordered_map<Config::DI_TYPE, Word> locals{};
    Config::DI_TYPE co{};
};

struct CallFrame {
    std::string name{};
    Config::DI_TYPE co{};
};

class Program {
public:
    std::unordered_map<std::string, Function> functions{};

    struct {
        std::string cf{};
        bool running = true;
        std::vector<CallFrame> call_stack{};
    } state;
};

class CIR {
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::vector<Word> stack{};

    std::unordered_map<std::string, CIR_ExternFn> extern_functions{};

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

    Word &getr(uint16_t i) {
        return registers[i];
    }

    Word &gets() {
        return stack.emplace_back();
    }

    void execute_op(Function &fn, Op op) { // TODO: add expects for types
        Word &dest = getr(0);
        switch (op.type) {
            case OpType::Mov: {
                move(op.args[0], op.args[1].as_int());
            } break;

            case OpType::Push: {
                push(op.args[0]);
            } break;

            case OpType::PushReg: {
                push(getr(op.args[0].as_int()));
            } break;

            case OpType::Pop: {
                Word &r = getr(op.args[0].as_int());
                r = pop();
            } break;

            case OpType::IAdd: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() + b.as_int());
            } break;

            case OpType::ISub: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() - b.as_int());
            } break;

            case OpType::IMul: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() * b.as_int());
            } break;

            case OpType::IDiv: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                if (b.as_int() == 0) {
                    throw std::runtime_error("Division by zero");
                }
                dest = Word::from_int(a.as_int() / b.as_int());
            } break;

            case OpType::IMod: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                if (b.as_int() == 0) {
                    throw std::runtime_error("Modulo by zero");
                }
                dest = Word::from_int(a.as_int() % b.as_int());
            } break;

            case OpType::And: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() & b.as_int());
            } break;

            case OpType::Or: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() | b.as_int());
            } break;

            case OpType::Xor: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() ^ b.as_int());
            } break;

            case OpType::Not: {
                Word &a = getr(op.args[0].as_int());
                dest = Word::from_int(~a.as_int());
            } break;

            case OpType::Shl: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() << b.as_int());
            } break;

            case OpType::Shr: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_int(a.as_int() >> b.as_int());
            } break;

            case OpType::ICmp: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                cmp_flag = (a.as_int() == b.as_int());
            } break;

            case OpType::Jmp: {
                fn.co = op.args[0].as_int();
            } break;

            case OpType::Je: {
                if (cmp_flag) {
                    fn.co = op.args[0].as_int();
                }
            } break;

            case OpType::Jne: {
                if (!cmp_flag) {
                    fn.co = op.args[0].as_int();
                }
            } break;

            case OpType::Inc: {
                Word &r = getr(op.args[0].as_int());
                r = Word::from_int(r.as_int() + 1);
            } break;

            case OpType::Dec: {
                Word &r = getr(op.args[0].as_int());
                r = Word::from_int(r.as_int() - 1);
            } break;

            case OpType::Neg: {
                Word &a = getr(op.args[0].as_int());
                dest = Word::from_int(-a.as_int());
            } break;

            case OpType::FAdd: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_float(a.as_float() + b.as_float());
            } break;

            case OpType::FSub: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_float(a.as_float() - b.as_float());
            } break;

            case OpType::FMul: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_float(a.as_float() * b.as_float());
            } break;

            case OpType::FDiv: {
                Word &a = getr(op.args[0].as_int());
                Word &b = getr(op.args[1].as_int());
                dest = Word::from_float(a.as_float() / b.as_float());
            } break;

            case OpType::Cast: {
                std::string target_type = (const char *)op.args[0].as_ptr();

                Word &a = getr(op.args[1].as_int());
                switch (a.type) {
                    case WordType::Integer: {
                        if (target_type == "int") {
                            break;
                        } if (target_type == "float") {
                            dest = Word::from_float(static_cast<double>(a.as_int()));
                        } if (target_type == "ptr") {
                            dest = Word::from_ptr((void*)a.as_int());
                        } else {
                            throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                        }
                    } break;
                    case WordType::Float: {
                        if (target_type == "int") {
                            dest = Word::from_int(static_cast<int>(a.as_float()));
                        } else if (target_type == "float") {
                            break;
                        } else {
                            throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                        }
                    } break;
                    case WordType::Pointer: {
                        if (target_type == "int") {
                            dest = Word::from_int((int64_t)a.as_ptr());
                        } else {
                            throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                        }
                    }
                    default: assert(0 && "Unsupported word type");
                }
            } break;

            case OpType::Halt: {
                program.state.running = false;
            } break;

            case OpType::Nop: break;

            case OpType::Call: {
                CallFrame cf = {program.state.cf, fn.co + 1};
                program.state.call_stack.push_back(cf);
                program.state.cf = std::string((const char *) op.args[0].as_ptr());

                if (!program.functions.contains(program.state.cf)) {
                    throw std::runtime_error("Function not found: " + program.state.cf);
                }
                program.functions[program.state.cf].co = 0;
            } return;

            case OpType::CallExtern: {
                if (op.args[0].type != WordType::Pointer) {
                    throw std::runtime_error("CallExtern: first argument must be a pointer to function name");
                }

                const char* fn_name_cstr = static_cast<const char*>(op.args[0].as_ptr());
                if (fn_name_cstr == nullptr) {
                    throw std::runtime_error("CallExtern: null function name");
                }

                std::string fn_name(fn_name_cstr);

                auto it = extern_functions.find(fn_name);
                if (it == extern_functions.end()) {
                    throw std::runtime_error("External function not found: " + fn_name);
                }

                it->second(*this);
            } break;

            case OpType::Ret: {
                if (program.state.call_stack.empty()) {
                    program.state.running = false;
                    return;
                }

                CallFrame cf = program.state.call_stack.back();
                program.state.call_stack.pop_back();

                program.state.cf = cf.name;
                program.functions[program.state.cf].co = cf.co;
            } return;

            // (local_id)
            case OpType::LocalGet: {
                Word::expect(op.args[0], WordType::Integer, "expecting local id");
                dest = fn.locals[op.args[0].as_int()];
            } break;

            // (local_id, value(reg))
            case OpType::LocalSet: {
                Word::expect(op.args[0], WordType::Integer, "expecting local id"); // local_id
                Word::expect(op.args[1], WordType::Integer, "expecting register"); // reg
                fn.locals[op.args[0].as_int()] = getr(op.args[1].as_int());
            } break;

            default: assert(0 && "wtf, this dont should happen.");
        }
    }

    void execute_function(const std::string &name) {
        program.state.cf = name;
        program.state.running = true;

        if (!program.functions.contains(name)) {
            throw std::runtime_error("Function not found: " + name);
        }

        program.functions[name].co = 0;

        while (program.state.running) {
            Function &fn = program.functions[program.state.cf];

            if (fn.co >= fn.ops.size()) {
                if (program.state.call_stack.empty()) {
                    program.state.running = false;
                    break;
                }

                CallFrame cf = program.state.call_stack.back();
                program.state.call_stack.pop_back();
                program.state.cf = cf.name;
                program.functions[program.state.cf].co = cf.co;
                continue;
            }

            execute_op(fn, fn.ops[fn.co]);
            fn.co++;
        }
    }

    void execute_program() {
        execute_function("main");
    }

std::vector<uint8_t> to_bytecode() {
    std::vector<uint8_t> bytes;

    std::unordered_map<std::string, uint32_t> string_table;
    std::vector<std::string> string_list;
    uint32_t string_index = 0;

    auto add_string = [&](const char* str) -> uint32_t {
        if (!str) return UINT32_MAX;
        std::string s(str);
        auto it = string_table.find(s);
        if (it != string_table.end()) {
            return it->second;
        }
        string_table[s] = string_index;
        string_list.push_back(s);
        return string_index++;
    };

    for (const auto &[name, func]: program.functions) {
        add_string(name.c_str());

        for (const auto &op: func.ops) {
            for (size_t i = 0; i < Config::OpArgCount; i++) {
                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer) {
                    add_string(static_cast<const char*>(op.args[i].data.p));
                }
            }
        }

        for (const auto &[local_id, local_val]: func.locals) {
            if (local_val.has_flag(WordFlag::String) && local_val.type == WordType::Pointer) {
                add_string(static_cast<const char*>(local_val.data.p));
            }
        }
    }

    uint32_t string_count = string_list.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&string_count),
                 reinterpret_cast<uint8_t*>(&string_count) + sizeof(string_count));

    for (const auto& str : string_list) {
        uint32_t str_len = str.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&str_len),
                     reinterpret_cast<uint8_t*>(&str_len) + sizeof(str_len));
        bytes.insert(bytes.end(), str.begin(), str.end());
        bytes.push_back(0);
    }

    uint32_t func_count = program.functions.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&func_count),
                 reinterpret_cast<uint8_t*>(&func_count) + sizeof(func_count));

    for (const auto &[name, func]: program.functions) {
        uint32_t name_idx = string_table[name];
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&name_idx),
                     reinterpret_cast<uint8_t*>(&name_idx) + sizeof(name_idx));

        uint32_t op_count = func.ops.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&op_count),
                     reinterpret_cast<uint8_t*>(&op_count) + sizeof(op_count));

        for (const auto &op: func.ops) {
            bytes.push_back(static_cast<uint8_t>(op.type));

            for (size_t i = 0; i < Config::OpArgCount; i++) {
                bytes.push_back(static_cast<uint8_t>(op.args[i].type));
                bytes.push_back(op.args[i].flags);

                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer) {
                    const char* str = static_cast<const char*>(op.args[i].data.p);
                    uint32_t str_idx = str ? string_table[std::string(str)] : UINT32_MAX;

                    bytes.insert(bytes.end(),
                                reinterpret_cast<uint8_t*>(&str_idx),
                                reinterpret_cast<uint8_t*>(&str_idx) + sizeof(str_idx));
                } else {
                    bytes.insert(bytes.end(),
                                reinterpret_cast<const uint8_t*>(&op.args[i].data),
                                reinterpret_cast<const uint8_t*>(&op.args[i].data) + sizeof(op.args[i].data));
                }
            }
        }

        uint32_t local_count = func.locals.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&local_count),
                     reinterpret_cast<uint8_t*>(&local_count) + sizeof(local_count));

        for (const auto &[local_id, local_val]: func.locals) {
            bytes.push_back(local_id);

            bytes.push_back(static_cast<uint8_t>(local_val.type));
            bytes.push_back(local_val.flags);

            if (local_val.has_flag(WordFlag::String) && local_val.type == WordType::Pointer) {
                const char* str = static_cast<const char*>(local_val.data.p);
                uint32_t str_idx = str ? string_table[std::string(str)] : UINT32_MAX;

                bytes.insert(bytes.end(),
                            reinterpret_cast<uint8_t*>(&str_idx),
                            reinterpret_cast<uint8_t*>(&str_idx) + sizeof(str_idx));
            } else {
                bytes.insert(bytes.end(),
                            reinterpret_cast<const uint8_t*>(&local_val.data),
                            reinterpret_cast<const uint8_t*>(&local_val.data) + sizeof(local_val.data));
            }
        }
    }

    return bytes;
}

void from_bytecode(const std::vector<uint8_t> &bytes) {
    size_t offset = 0;
    program = Program{};

    if (bytes.size() < sizeof(uint32_t)) {
        throw std::runtime_error("Bytecode too short: cannot read string count");
    }

    uint32_t string_count;
    std::memcpy(&string_count, &bytes[offset], sizeof(string_count));
    offset += sizeof(string_count);

    std::vector<std::string> string_table(string_count);

    for (uint32_t s = 0; s < string_count; s++) {
        if (offset + sizeof(uint32_t) > bytes.size()) {
            throw std::runtime_error("Bytecode truncated: cannot read string length");
        }

        uint32_t str_len;
        std::memcpy(&str_len, &bytes[offset], sizeof(str_len));
        offset += sizeof(str_len);

        if (offset + str_len + 1 > bytes.size()) {
            throw std::runtime_error("Bytecode truncated: cannot read string data");
        }

        string_table[s] = std::string(reinterpret_cast<const char*>(&bytes[offset]), str_len);
        offset += str_len + 1;
    }

    if (offset + sizeof(uint32_t) > bytes.size()) {
        throw std::runtime_error("Bytecode too short: cannot read function count");
    }

    uint32_t func_count;
    std::memcpy(&func_count, &bytes[offset], sizeof(func_count));
    offset += sizeof(func_count);

    for (uint32_t f = 0; f < func_count; f++) {
        if (offset + sizeof(uint32_t) > bytes.size()) {
            throw std::runtime_error("Bytecode truncated: cannot read function name index");
        }

        uint32_t name_idx;
        std::memcpy(&name_idx, &bytes[offset], sizeof(name_idx));
        offset += sizeof(name_idx);

        if (name_idx >= string_table.size()) {
            throw std::runtime_error("Invalid string table index for function name");
        }

        const std::string& func_name = string_table[name_idx];

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
                if (offset + 2 > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read op argument type and flags");
                }

                op.args[i].type = static_cast<WordType>(bytes[offset++]);
                op.args[i].flags = bytes[offset++];

                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer) {
                    if (offset + sizeof(uint32_t) > bytes.size()) {
                        throw std::runtime_error("Bytecode truncated: cannot read string index");
                    }

                    uint32_t str_idx;
                    std::memcpy(&str_idx, &bytes[offset], sizeof(str_idx));
                    offset += sizeof(str_idx);

                    if (str_idx == UINT32_MAX) {
                        op.args[i].data.p = nullptr;
                    } else {
                        if (str_idx >= string_table.size()) {
                            throw std::runtime_error("Invalid string table index");
                        }

                        const std::string& str = string_table[str_idx];
                        char* str_copy = new char[str.size() + 1];
                        std::strcpy(str_copy, str.c_str());

                        op.args[i].data.p = str_copy;
                        op.args[i].set_flag(WordFlag::OwnsMemory);
                    }
                } else {
                    if (offset + sizeof(op.args[i].data) > bytes.size()) {
                        throw std::runtime_error("Bytecode truncated: cannot read op argument data");
                    }

                    std::memcpy(&op.args[i].data, &bytes[offset], sizeof(op.args[i].data));
                    offset += sizeof(op.args[i].data);
                }
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
                throw std::runtime_error("Bytecode truncated: cannot read local id");
            }

            uint32_t local_id;
            std::memcpy(&local_id, &bytes[offset], sizeof(Config::DI_TYPE));
            offset += sizeof(Config::DI_TYPE);

            if (offset + 2 > bytes.size()) {
                throw std::runtime_error("Bytecode truncated: cannot read local value type and flags");
            }

            Word local_val;
            local_val.type = static_cast<WordType>(bytes[offset++]);
            local_val.flags = bytes[offset++];

            if (local_val.has_flag(WordFlag::String) && local_val.type == WordType::Pointer) {
                if (offset + sizeof(uint32_t) > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read string index");
                }

                uint32_t str_idx;
                std::memcpy(&str_idx, &bytes[offset], sizeof(str_idx));
                offset += sizeof(str_idx);

                if (str_idx == UINT32_MAX) {
                    local_val.data.p = nullptr;
                } else {
                    if (str_idx >= string_table.size()) {
                        throw std::runtime_error("Invalid string table index");
                    }

                    const std::string& str = string_table[str_idx];
                    char* str_copy = new char[str.size() + 1];
                    std::strcpy(str_copy, str.c_str());

                    local_val.data.p = str_copy;
                    local_val.set_flag(WordFlag::OwnsMemory);
                }
            } else {
                if (offset + sizeof(Word::data) > bytes.size()) {
                    throw std::runtime_error("Bytecode truncated: cannot read local value data");
                }

                std::memcpy(&local_val.data, &bytes[offset], sizeof(local_val.data));
                offset += sizeof(local_val.data);
            }

            func.locals[local_id] = local_val;
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

    void set_extern_fn(std::string n, CIR_ExternFn f) {
        extern_functions[n] = f;
    }

    std::vector<Word> &get_stack() {
        return stack;
    }
};