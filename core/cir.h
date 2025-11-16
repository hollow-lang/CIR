#ifndef CIR_AS_LIB
#pragma once
#endif

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "config.h"
#include "helpers/heap.h"

#ifndef CIR_API
    #ifdef CIR_STATIC
        #define CIR_API
    #elif defined(_WIN32) || defined(_WIN64)
        #ifdef CIR_BUILD_DLL
            #define CIR_API __declspec(dllexport)
        #else
            #define CIR_API __declspec(dllimport)
        #endif
    #elif defined(__GNUC__) && __GNUC__ >= 4
        #ifdef CIR_BUILD_DLL
            #define CIR_API __attribute__((visibility("default")))
        #else
            #define CIR_API
        #endif
    #else
        #define CIR_API
    #endif
#endif

#ifndef CIR_INTERNAL
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define CIR_INTERNAL __attribute__((visibility("hidden")))
    #else
        #define CIR_INTERNAL
    #endif
#endif


class CIR;

using CIR_ExternFn = void (*)(CIR &vm);
using CIR_InitLibFn = void (*)(CIR &vm);


enum class WordType : uint8_t {
    Integer,
    Float,
    Pointer,
    Boolean,
    Null
};

enum class WordFlag : uint16_t {
    None = 0,
    String = 1 << 1,
    OwnsMemory = 1 << 2,
    Register = 1 << 3,
};

struct Word {
    WordType type{WordType::Null};
    uint16_t flags = 0;

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

    void print() const;


    static Word from_int(int64_t val) {
        Word w;
        w.type = WordType::Integer;
        w.data.i = val;
        return w;
    }

    static Word from_reg(int64_t val) {
        Word w;
        w.type = WordType::Integer;
        w.data.i = val;
        w.set_flag(WordFlag::Register);
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

    void set_flag(WordFlag flag) { flags |= static_cast<uint8_t>(flag); }

    [[nodiscard]] bool has_flag(WordFlag flag) const { return (flags & static_cast<uint8_t>(flag)) != 0; }

    [[nodiscard]] int64_t as_int() const { return data.i; }
    [[nodiscard]] double as_float() const { return data.f; }
    [[nodiscard]] void *as_ptr() const { return data.p; }
    [[nodiscard]] bool as_bool() const { return data.b; }

    constexpr static void expect(Word &w, WordType type, const char *msg) {
        if (w.type != type) {
            throw std::runtime_error("Expected " + std::to_string(static_cast<int>(type)) + " but got " +
                                     std::to_string(static_cast<int>(w.type)) + ": " + msg);
        }
    }
};

// TODO: pointer operations (PAdd, PSub)
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
    IAnd,
    IOr,
    IXor,
    Not,
    Shl,
    Shr,
    ICmp,
    Jmp,
    Je,
    Jne,
    Gt,
    Lt,
    Gte,
    Lte,
    Call,
    CallExtern,
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
    Cast,
    LocalGet,
    LocalSet,
    Alloc,
    Free,
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

    std::vector<std::string> required_externs{};

    struct {
        std::string cf{};
        bool running = true;
        std::vector<CallFrame> call_stack{};
    } state;
    void optimize();
};

class CIR {
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::vector<Word> stack{};
    std::unordered_map<std::string, CIR_ExternFn> extern_functions{};
    bool cmp_flag{false};
    Program program;
    Heap heap{Config::HEAP_SIZE};

public:
    Word pop();

    void push(const Word &value);

    void move(const Word &w, uint16_t i);

    Word &getr(uint16_t i);

    Word &gets();

    void execute_op(Function &fn, Op op);

    void execute_function(const std::string &name);

    void check_externs();

    void execute_program();

    std::vector<uint8_t> to_bytecode();

    void from_bytecode(const std::vector<uint8_t> &bytes);

    void load_program(Program p);

    Program &get_program();

    void set_extern_fn(std::string n, CIR_ExternFn f);

    std::vector<Word> &get_stack();
};

#ifdef CIR_IMPLEMENTATION

void Word::print() const {
    switch (type) {
        case WordType::Integer:
            if (has_flag(WordFlag::Register)) std::cout << "r" << as_int();
            else std::cout << as_int();
            break;
        case WordType::Float:
            std::cout << std::fixed << std::setprecision(2) << as_float();
            break;
        case WordType::Pointer:
            if (has_flag(WordFlag::String)) {
                std::cout << static_cast<char *>(as_ptr());
            } else {
                std::cout << as_ptr();
            }
            break;
        case WordType::Boolean:
            std::cout << (as_bool() ? "true" : "false");
            break;
        case WordType::Null:
            std::cout << "null";
            break;
    }
}

CIR_API Word CIR::pop() {
    Word top = stack.back();
    stack.pop_back();
    return top;
}

CIR_API void CIR::push(const Word &value) {
    stack.push_back(value);
}

CIR_API void CIR::move(const Word &w, uint16_t i) {
    registers[i] = w;
}

CIR_API Word &CIR::getr(uint16_t i) {
    return registers[i];
}

CIR_API Word &CIR::gets() {
    return stack.emplace_back();
}

// TODO: add expect for types
CIR_API void CIR::execute_op(Function &fn, Op op) {
    Word &dest = getr(0);
    switch (op.type) {
        case OpType::Mov: {
            Word value;
            if (op.args[0].has_flag(WordFlag::Register)) {
                value = getr(op.args[0].as_int());
            } else {
                value = op.args[0];
            }
            move(value, op.args[1].as_int());
        }
        break;

        case OpType::Push: {
            push(op.args[0]);
        }
        break;

        case OpType::PushReg: {
            push(getr(op.args[0].as_int()));
        }
        break;

        case OpType::Pop: {
            Word &r = getr(op.args[0].as_int());
            r = pop();
        }
        break;

        // TODO: add checks for registers so add ability for IAdd literal, literal @enchancement
        case OpType::IAdd: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() + b.as_int());
        }
        break;

        // TODO: add checks for registers so add ability for IAdd literal, literal @enchancement
        case OpType::ISub: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() - b.as_int());
        }
        break;

        // TODO: add checks for registers so add ability for IAdd literal, literal @enchancement
        case OpType::IMul: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() * b.as_int());
        }
        break;

        // TODO: add checks for registers so add ability for IAdd literal, literal @enchancement
        case OpType::IDiv: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            if (b.as_int() == 0) {
                throw std::runtime_error("Division by zero");
            }
            dest = Word::from_int(a.as_int() / b.as_int());
        }
        break;

        case OpType::IMod: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            if (b.as_int() == 0) {
                throw std::runtime_error("Modulo by zero");
            }
            dest = Word::from_int(a.as_int() % b.as_int());
        }
        break;

        case OpType::IAnd: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() & b.as_int());
        }
        break;

        case OpType::IOr: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() | b.as_int());
        }
        break;

        case OpType::IXor: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() ^ b.as_int());
        }
        break;

        case OpType::Not: {
            Word &a = getr(op.args[0].as_int());
            dest = Word::from_int(~a.as_int());
        }
        break;

        case OpType::Shl: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() << b.as_int());
        }
        break;

        case OpType::Shr: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_int(a.as_int() >> b.as_int());
        }
        break;

        case OpType::ICmp: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            cmp_flag = (a.as_int() == b.as_int());
        }
        break;

        case OpType::Jmp: {
            fn.co = op.args[0].as_int();
        }
        break;

        case OpType::Je: {
            if (cmp_flag) {
                fn.co = op.args[0].as_int();
            }
        }
        break;

        case OpType::Jne: {
            if (!cmp_flag) {
                fn.co = op.args[0].as_int();
            }
        }
        break;

        case OpType::Gt: {
            cmp_flag = (getr(op.args[0].as_int()).as_int() > getr(op.args[1].as_int()).as_int());
        }
        break;

        case OpType::Lt: {
            cmp_flag = (getr(op.args[0].as_int()).as_int() < getr(op.args[1].as_int()).as_int());
        }
        break;

        case OpType::Gte: {
            cmp_flag = (getr(op.args[0].as_int()).as_int() >= getr(op.args[1].as_int()).as_int());
        }
        break;

        case OpType::Lte: {
            cmp_flag = (getr(op.args[0].as_int()).as_int() <= getr(op.args[1].as_int()).as_int());
        }
        break;

        case OpType::Inc: {
            Word &r = getr(op.args[0].as_int());
            r = Word::from_int(r.as_int() + 1);
        }
        break;

        case OpType::Dec: {
            Word &r = getr(op.args[0].as_int());
            r = Word::from_int(r.as_int() - 1);
        }
        break;

        case OpType::Neg: {
            Word &a = getr(op.args[0].as_int());
            dest = Word::from_int(-a.as_int());
        }
        break;

        case OpType::FAdd: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_float(a.as_float() + b.as_float());
        }
        break;

        case OpType::FSub: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_float(a.as_float() - b.as_float());
        }
        break;

        case OpType::FMul: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_float(a.as_float() * b.as_float());
        }
        break;

        case OpType::FDiv: {
            Word &a = getr(op.args[0].as_int());
            Word &b = getr(op.args[1].as_int());
            dest = Word::from_float(a.as_float() / b.as_float());
        }
        break;

        case OpType::Cast: {
            std::string target_type = (const char *) op.args[0].as_ptr();

            Word &a = getr(op.args[1].as_int());
            switch (a.type) {
                case WordType::Integer: {
                    if (target_type == "int") {
                        break;
                    }
                    if (target_type == "float") {
                        dest = Word::from_float(static_cast<double>(a.as_int()));
                    }
                    if (target_type == "ptr") {
                        dest = Word::from_ptr((void *) a.as_int());
                    } else {
                        throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                    }
                }
                break;
                case WordType::Float: {
                    if (target_type == "int") {
                        dest = Word::from_int(static_cast<int>(a.as_float()));
                    } else if (target_type == "float") {
                        break;
                    } else {
                        throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                    }
                }
                break;
                case WordType::Pointer: {
                    if (target_type == "int") {
                        dest = Word::from_int((int64_t) a.as_ptr());
                    } else {
                        throw std::runtime_error("Invalid cast type: " + std::string(target_type));
                    }
                }
                break;
                default: assert(0 && "Unsupported word type");
            }
        }
        break;

        case OpType::Halt: program.state.running = false;
            break;

        case OpType::Nop: break;

        case OpType::Call: {
            CallFrame cf = {program.state.cf, fn.co + 1};
            program.state.call_stack.push_back(cf);
            program.state.cf = std::string((const char *) op.args[0].as_ptr());

            if (!program.functions.contains(program.state.cf)) {
                throw std::runtime_error("Function not found: " + program.state.cf);
            }
            program.functions[program.state.cf].co = 0;
        }
            return;

        case OpType::CallExtern: {
            if (op.args[0].type != WordType::Pointer) {
                throw std::runtime_error("CallExtern: first argument must be a pointer to function name");
            }

            const char *fn_name_cstr = static_cast<const char *>(op.args[0].as_ptr());
            if (fn_name_cstr == nullptr) {
                throw std::runtime_error("CallExtern: null function name");
            }

            std::string fn_name(fn_name_cstr);

            auto it = extern_functions.find(fn_name);
            if (it == extern_functions.end()) {
                throw std::runtime_error("External function not found: " + fn_name);
            }

            it->second(*this);
        }
        break;

        case OpType::Ret: {
            if (program.state.call_stack.empty()) {
                program.state.running = false;
                return;
            }

            CallFrame cf = program.state.call_stack.back();
            program.state.call_stack.pop_back();

            program.state.cf = cf.name;
            program.functions[program.state.cf].co = cf.co;
        }
            return;

        case OpType::LocalGet: {
            Word::expect(op.args[0], WordType::Integer, "expecting local id");
            dest = fn.locals[op.args[0].as_int()];
        }
        break;

        case OpType::LocalSet: {
            Word::expect(op.args[0], WordType::Integer, "expecting local id");
            Word::expect(op.args[1], WordType::Integer, "expecting register");
            fn.locals[op.args[0].as_int()] = getr(op.args[1].as_int());
        }
        break;

        case OpType::FCmp: {
            cmp_flag = (getr(op.args[0].as_int()).as_float() == getr(op.args[1].as_int()).as_float());
        }
        break;

        case OpType::Load: {
            void *d = getr(op.args[0].as_int()).as_ptr();
            void *src = getr(op.args[1].as_int()).as_ptr();
            if (!src) throw std::runtime_error("Load: source pointer is null");
            if (!d) throw std::runtime_error("Load: destionation pointer is null");

            memcpy(d, src, op.args[2].as_int());
        }
        break;

        case OpType::Store: {
            memcpy(getr(op.args[0].as_int()).as_ptr(), getr(op.args[1].as_int()).as_ptr(), op.args[2].as_int());
        }
        break;

        case OpType::Alloc: {
            dest = Word::from_ptr(heap.allocate(op.args[0].as_int()));
        }
        break;

        case OpType::Free: {
            heap.deallocate(getr(op.args[0].as_int()).as_ptr());
        }
        break;

        default: assert(0 && "wtf, this dont should happen.");
    }
}

CIR_API void CIR::execute_function(const std::string &name) {
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

CIR_API void CIR::check_externs() {
    for (const auto &req: program.required_externs) {
        if (!extern_functions.contains(req)) {
            throw std::runtime_error("Missing required external function: " + req);
        }
    }
}

CIR_API void CIR::execute_program() {
    check_externs();
    execute_function("main");
}

CIR_API std::vector<uint8_t> CIR::to_bytecode() {
    std::vector<uint8_t> bytes;

    std::unordered_map<std::string, uint32_t> string_table;
    std::vector<std::string> string_list;
    uint32_t string_index = 0;

    auto add_string = [&](const char *str) -> uint32_t {
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
                    add_string(static_cast<const char *>(op.args[i].data.p));
                }
            }
        }

        for (const auto &[local_id, local_val]: func.locals) {
            if (local_val.has_flag(WordFlag::String) && local_val.type == WordType::Pointer) {
                add_string(static_cast<const char *>(local_val.data.p));
            }
        }
    }

    for (const auto &req: program.required_externs) {
        add_string(req.c_str());
    }

    uint32_t string_count = string_list.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&string_count),
                 reinterpret_cast<uint8_t *>(&string_count) + sizeof(string_count));

    for (const auto &str: string_list) {
        uint32_t str_len = str.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&str_len),
                     reinterpret_cast<uint8_t *>(&str_len) + sizeof(str_len));
        bytes.insert(bytes.end(), str.begin(), str.end());
        bytes.push_back(0);
    }

    uint32_t req_count = program.required_externs.size();
    bytes.insert(bytes.end(),
                 reinterpret_cast<uint8_t *>(&req_count),
                 reinterpret_cast<uint8_t *>(&req_count) + sizeof(req_count));

    for (const auto &req: program.required_externs) {
        uint32_t str_idx = string_table[req];
        bytes.insert(bytes.end(),
                     reinterpret_cast<uint8_t *>(&str_idx),
                     reinterpret_cast<uint8_t *>(&str_idx) + sizeof(str_idx));
    }

    uint32_t func_count = program.functions.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&func_count),
                 reinterpret_cast<uint8_t *>(&func_count) + sizeof(func_count));

    for (const auto &[name, func]: program.functions) {
        uint32_t name_idx = string_table[name];
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&name_idx),
                     reinterpret_cast<uint8_t *>(&name_idx) + sizeof(name_idx));

        uint32_t op_count = func.ops.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&op_count),
                     reinterpret_cast<uint8_t *>(&op_count) + sizeof(op_count));

        for (const auto &op: func.ops) {
            bytes.push_back(static_cast<uint8_t>(op.type));

            for (size_t i = 0; i < Config::OpArgCount; i++) {
                bytes.push_back(static_cast<uint8_t>(op.args[i].type));
                bytes.push_back(op.args[i].flags);

                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer) {
                    const char *str = static_cast<const char *>(op.args[i].data.p);
                    uint32_t str_idx = str ? string_table[std::string(str)] : UINT32_MAX;

                    bytes.insert(bytes.end(),
                                 reinterpret_cast<uint8_t *>(&str_idx),
                                 reinterpret_cast<uint8_t *>(&str_idx) + sizeof(str_idx));
                } else {
                    bytes.insert(bytes.end(),
                                 reinterpret_cast<const uint8_t *>(&op.args[i].data),
                                 reinterpret_cast<const uint8_t *>(&op.args[i].data) + sizeof(op.args[i].data));
                }
            }
        }

        uint32_t local_count = func.locals.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t *>(&local_count),
                     reinterpret_cast<uint8_t *>(&local_count) + sizeof(local_count));

        for (const auto &[local_id, local_val]: func.locals) {
            bytes.push_back(local_id);

            bytes.push_back(static_cast<uint8_t>(local_val.type));
            bytes.push_back(local_val.flags);

            if (local_val.has_flag(WordFlag::String) && local_val.type == WordType::Pointer) {
                const char *str = static_cast<const char *>(local_val.data.p);
                uint32_t str_idx = str ? string_table[std::string(str)] : UINT32_MAX;

                bytes.insert(bytes.end(),
                             reinterpret_cast<uint8_t *>(&str_idx),
                             reinterpret_cast<uint8_t *>(&str_idx) + sizeof(str_idx));
            } else {
                bytes.insert(bytes.end(),
                             reinterpret_cast<const uint8_t *>(&local_val.data),
                             reinterpret_cast<const uint8_t *>(&local_val.data) + sizeof(local_val.data));
            }
        }
    }

    return bytes;
}

CIR_API void CIR::from_bytecode(const std::vector<uint8_t> &bytes) {
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

        string_table[s] = std::string(reinterpret_cast<const char *>(&bytes[offset]), str_len);
        offset += str_len + 1;
    }

    if (offset + sizeof(uint32_t) > bytes.size()) {
        throw std::runtime_error("Bytecode truncated: cannot read required_externs count");
    }

    uint32_t req_count;
    std::memcpy(&req_count, &bytes[offset], sizeof(req_count));
    offset += sizeof(req_count);

    program.required_externs.clear();
    for (uint32_t i = 0; i < req_count; i++) {
        if (offset + sizeof(uint32_t) > bytes.size()) {
            throw std::runtime_error("Bytecode truncated: cannot read required_externs string index");
        }

        uint32_t str_idx;
        std::memcpy(&str_idx, &bytes[offset], sizeof(str_idx));
        offset += sizeof(str_idx);

        if (str_idx >= string_table.size()) {
            throw std::runtime_error("Invalid string table index for required_extern");
        }

        program.required_externs.push_back(string_table[str_idx]);
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

        const std::string &func_name = string_table[name_idx];

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

                        const std::string &str = string_table[str_idx];
                        char *str_copy = new char[str.size() + 1];
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

                    const std::string &str = string_table[str_idx];
                    char *str_copy = new char[str.size() + 1];
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

CIR_API void CIR::load_program(Program p) {
    program = std::move(p);
}

CIR_API Program &CIR::get_program() {
    return program;
}

CIR_API void CIR::set_extern_fn(std::string n, CIR_ExternFn f) {
    extern_functions[n] = f;
}

CIR_API std::vector<Word> &CIR::get_stack() {
    return stack;
}

CIR_API void Program::optimize() {
    for (auto &[name, func]: functions) {
        bool changed = true;
        int pass = 0;
        const int MAX_PASSES = 5;

        while (changed && pass < MAX_PASSES) {
            changed = false;
            pass++;

            std::vector<Op> optimized_ops;
            optimized_ops.reserve(func.ops.size());

            std::unordered_map<int64_t, Word> reg_values;

            auto is_int_imm = [](const Word &w) -> bool {
                return w.type == WordType::Integer && !w.has_flag(WordFlag::Register);
            };
            auto is_float_imm = [](const Word &w) -> bool {
                return w.type == WordType::Float && !w.has_flag(WordFlag::Register);
            };
            auto is_register = [](const Word &w) -> bool {
                return w.has_flag(WordFlag::Register);
            };

            for (size_t i = 0; i < func.ops.size(); ++i) {
                const Op &op = func.ops[i];

                if (op.type == OpType::Nop) {
                    changed = true;
                    continue;
                }

                if (op.type == OpType::Halt) {
                    optimized_ops.push_back(op);
                    bool has_jump_past = false;
                    for (size_t j = 0; j < func.ops.size(); ++j) {
                        const Op &check_op = func.ops[j];
                        if (check_op.type == OpType::Jmp || check_op.type == OpType::Je ||
                            check_op.type == OpType::Jne) {
                            int64_t target = check_op.args[0].as_int();
                            if (target > static_cast<int64_t>(i)) {
                                has_jump_past = true;
                                break;
                            }
                        }
                    }
                    if (!has_jump_past) {
                        changed = true;
                        break;
                    }
                    continue;
                }

                if ((op.type == OpType::IAdd || op.type == OpType::ISub || op.type == OpType::IMul ||
                     op.type == OpType::IDiv || op.type == OpType::IMod ||
                     op.type == OpType::IAnd || op.type == OpType::IOr || op.type == OpType::IXor ||
                     op.type == OpType::Shl || op.type == OpType::Shr)) {

                    Word arg0 = op.args[0];
                    Word arg1 = op.args[1];

                    if (is_register(arg0) && reg_values.count(arg0.as_int())) {
                        arg0 = reg_values[arg0.as_int()];
                    }
                    if (is_register(arg1) && reg_values.count(arg1.as_int())) {
                        arg1 = reg_values[arg1.as_int()];
                    }

                    if (is_int_imm(arg0) && is_int_imm(arg1)) {
                        int64_t a = arg0.data.i;
                        int64_t b = arg1.data.i;
                        bool fold_ok = true;
                        int64_t result = 0;

                        switch (op.type) {
                            case OpType::IAdd: result = a + b; break;
                            case OpType::ISub: result = a - b; break;
                            case OpType::IMul: result = a * b; break;
                            case OpType::IDiv:
                                if (b == 0) fold_ok = false;
                                else result = a / b;
                                break;
                            case OpType::IMod:
                                if (b == 0) fold_ok = false;
                                else result = a % b;
                                break;
                            case OpType::IAnd: result = a & b; break;
                            case OpType::IOr:  result = a | b; break;
                            case OpType::IXor: result = a ^ b; break;
                            case OpType::Shl:  result = a << b; break;
                            case OpType::Shr:  result = a >> b; break;
                            default: fold_ok = false; break;
                        }

                        if (fold_ok) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = Word::from_int(result);
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            reg_values[0] = Word::from_int(result);
                            changed = true;
                            continue;
                        }
                    }

                    if (is_int_imm(arg1)) {
                        int64_t b = arg1.data.i;

                        if ((op.type == OpType::IAdd || op.type == OpType::ISub ||
                             op.type == OpType::IOr || op.type == OpType::IXor) && b == 0) {
                            if (is_register(arg0)) {
                                Op mov{};
                                mov.type = OpType::Mov;
                                mov.args[0] = arg0;
                                mov.args[1] = Word::from_int(0);
                                optimized_ops.push_back(std::move(mov));
                            } else {
                                Op mov{};
                                mov.type = OpType::Mov;
                                mov.args[0] = arg0;
                                mov.args[1] = Word::from_int(0);
                                optimized_ops.push_back(std::move(mov));
                            }
                            changed = true;
                            continue;
                        }

                        if ((op.type == OpType::IMul || op.type == OpType::IAnd) && b == 0) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = Word::from_int(0);
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            reg_values[0] = Word::from_int(0);
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::IMul && b == 1) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = arg0;
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::IMul && b > 0 && (b & (b - 1)) == 0) {
                            int shift = 0;
                            int64_t temp = b;
                            while (temp > 1) { temp >>= 1; shift++; }

                            Op shl{};
                            shl.type = OpType::Shl;
                            shl.args[0] = arg0;
                            shl.args[1] = Word::from_int(shift);
                            optimized_ops.push_back(std::move(shl));
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::IDiv && b == 1) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = arg0;
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::IAnd && b == -1) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = arg0;
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::IXor && is_register(arg0) && is_register(op.args[1]) &&
                            arg0.as_int() == op.args[1].as_int()) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = Word::from_int(0);
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            reg_values[0] = Word::from_int(0);
                            changed = true;
                            continue;
                        }
                    }
                }

                if ((op.type == OpType::FAdd || op.type == OpType::FSub ||
                     op.type == OpType::FMul || op.type == OpType::FDiv)) {

                    Word arg0 = op.args[0];
                    Word arg1 = op.args[1];

                    if (is_register(arg0) && reg_values.count(arg0.as_int())) {
                        arg0 = reg_values[arg0.as_int()];
                    }
                    if (is_register(arg1) && reg_values.count(arg1.as_int())) {
                        arg1 = reg_values[arg1.as_int()];
                    }

                    if (is_float_imm(arg0) && is_float_imm(arg1)) {
                        double a = arg0.data.f;
                        double b = arg1.data.f;
                        bool fold_ok = true;
                        double result = 0.0;

                        switch (op.type) {
                            case OpType::FAdd: result = a + b; break;
                            case OpType::FSub: result = a - b; break;
                            case OpType::FMul: result = a * b; break;
                            case OpType::FDiv:
                                if (b == 0.0) fold_ok = false;
                                else result = a / b;
                                break;
                            default: fold_ok = false; break;
                        }

                        if (fold_ok) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = Word::from_float(result);
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            reg_values[0] = Word::from_float(result);
                            changed = true;
                            continue;
                        }
                    }

                    if (is_float_imm(arg1)) {
                        double b = arg1.data.f;

                        if ((op.type == OpType::FAdd || op.type == OpType::FSub) && b == 0.0) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = arg0;
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            changed = true;
                            continue;
                        }

                        if ((op.type == OpType::FMul || op.type == OpType::FDiv) && b == 1.0) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = arg0;
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            changed = true;
                            continue;
                        }

                        if (op.type == OpType::FMul && b == 0.0) {
                            Op mov{};
                            mov.type = OpType::Mov;
                            mov.args[0] = Word::from_float(0.0);
                            mov.args[1] = Word::from_int(0);
                            optimized_ops.push_back(std::move(mov));
                            reg_values[0] = Word::from_float(0.0);
                            changed = true;
                            continue;
                        }
                    }
                }

                if (op.type == OpType::Mov) {
                    const Word &src = op.args[0];
                    const Word &dst = op.args[1];

                    if (is_register(src) && is_int_imm(dst)) {
                        int64_t src_idx = src.as_int();
                        int64_t dst_idx = dst.as_int();
                        if (src_idx == dst_idx) {
                            changed = true;
                            continue;
                        }
                    }

                    if (is_int_imm(dst)) {
                        int64_t reg_idx = dst.as_int();
                        if (!is_register(src)) {
                            reg_values[reg_idx] = src;
                        } else {
                            reg_values.erase(reg_idx);
                        }
                    }
                }

                if ((op.type == OpType::Inc || op.type == OpType::Dec) &&
                    i + 1 < func.ops.size()) {
                    const Op &next = func.ops[i + 1];

                    if ((op.type == OpType::Inc && next.type == OpType::Dec) ||
                        (op.type == OpType::Dec && next.type == OpType::Inc)) {
                        if (is_int_imm(op.args[0]) && is_int_imm(next.args[0]) &&
                            op.args[0].as_int() == next.args[0].as_int()) {
                            i++;
                            changed = true;
                            continue;
                        }
                    }
                }

                if (op.type == OpType::Mov && !is_register(op.args[0]) && is_int_imm(op.args[1]) &&
                    i + 1 < func.ops.size()) {
                    const Op &next = func.ops[i + 1];

                    if ((next.type == OpType::IAdd || next.type == OpType::ISub) &&
                        is_register(next.args[0]) && is_int_imm(next.args[1]) &&
                        next.args[0].as_int() == op.args[1].as_int() &&
                        !is_register(next.args[1])) {

                        int64_t val1 = op.args[0].as_int();
                        int64_t val2 = next.args[1].as_int();
                        int64_t result = (next.type == OpType::IAdd) ? (val1 + val2) : (val1 - val2);

                        Op mov{};
                        mov.type = OpType::Mov;
                        mov.args[0] = Word::from_int(result);
                        mov.args[1] = op.args[1];
                        optimized_ops.push_back(std::move(mov));
                        reg_values[op.args[1].as_int()] = Word::from_int(result);
                        i++;
                        changed = true;
                        continue;
                    }
                }

                if (is_int_imm(op.args[1])) {
                    int64_t reg_idx = op.args[1].as_int();
                    if (op.type != OpType::Mov) {
                        reg_values.erase(reg_idx);
                    }
                }

                optimized_ops.push_back(op);
            }

            func.ops = std::move(optimized_ops);
        }
    }
}


#endif
