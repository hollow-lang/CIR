// TODO: implement syscall (maybe via registers?)

#ifndef CIR_AS_LIB
#pragma once
#endif

#include <array>
#include <cstdint>
#include <cstring>
#include <assert.h>
#include <iomanip>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <stdexcept>
#include <iostream>

#include "word.h"
#include "config.h"
#include "helpers/heap.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#elif defined(_WIN32) || defined(_WIN64)
#endif

#ifndef CIR_API
#   ifdef CIR_STATIC
#       define CIR_API
#   elif defined(_WIN32) || defined(_WIN64)
#       ifdef CIR_BUILD_DLL
#           define CIR_API __declspec(dllexport)
#       else
#           define CIR_API __declspec(dllimport)
#       endif
#   elif defined(__GNUC__) && __GNUC__ >= 4
#       ifdef CIR_BUILD_DLL
#           define CIR_API __attribute__((visibility("default")))
#       else
#           define CIR_API
#       endif
#   else
#       define CIR_API
#   endif
#endif

#ifndef CIR_INTERNAL
#   if defined(__GNUC__) && __GNUC__ >= 4
#       define CIR_INTERNAL __attribute__((visibility("hidden")))
#   else
#       define CIR_INTERNAL
#   endif
#endif

#ifndef CIR_INLINE
#   if defined(__GCC__) && __GCC__ >= 4
#       define CIR_INLINE inline __attribute__((always_inline))
#   else
#       define CIR_INLINE inline
#   endif
#endif


class CIR;

using CIR_ExternFn = void (*)(CIR& vm);
using CIR_InitLibFn = void (*)(CIR& vm);

enum class OpType : uint8_t
{
    Mov,
    Load,
    Store,
    Push,
    Pop,
    Lea,
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
    Inc,
    Dec,
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
    Lt,
    Gt,
    Lte,
    Gte,
    Call,
    CallExtern,
    Ret,
    Cast,
    Alloc,
    Free,
    Nop,
    Halt,
};

struct Op
{
    OpType type{};
    std::array<Word, Config::OpArgCount> args{};
};

struct Function
{
    std::vector<Op> ops{};
    Config::DI_TYPE co{};
};

struct CallFrame
{
    std::string name{};
    Config::DI_TYPE co{};
};

class Program
{
public:
    std::unordered_map<std::string, Function> functions{};

    std::vector<std::string> required_externs{};

    struct
    {
        std::string cf{};
        bool running = true;
        std::vector<CallFrame> call_stack{};
    } state;

    void optimize();
};

class CIR
{
    std::array<Word, Config::REGISTER_COUNT> registers{};
    std::vector<Word> stack{};
    std::unordered_map<std::string, CIR_ExternFn> extern_functions{};
    bool cmp_flag{false};
    Program program;
    Heap heap{Config::HEAP_SIZE};

public:
    Word pop();

    void push(const Word& value);

    void move(const Word& w, uint16_t i);

    Word& getr(uint16_t i);

    Word& gets();

    void execute_op(Function& fn, Op op);

    void execute_function(const std::string& name);

    void check_externs();

    void execute_program();

    std::vector<uint8_t> to_bytecode();

    void from_bytecode(const std::vector<uint8_t>& bytes);

    void load_program(Program p);

    Program& get_program();

    void set_extern_fn(std::string n, CIR_ExternFn f);

    std::vector<Word>& get_stack();

    Word& go(Word& w);
};

#ifdef CIR_IMPLEMENTATION

void Word::print() const
{
    switch (type)
    {
    case WordType::Integer:
        if (has_flag(WordFlag::Register)) std::cout << "r" << as_int();
        else std::cout << as_int();
        break;
    case WordType::Float:
        std::cout << std::fixed << std::setprecision(2) << as_float();
        break;
    case WordType::Pointer:
        if (has_flag(WordFlag::String))
        {
            std::cout << static_cast<char*>(as_ptr());
        }
        else
        {
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

CIR_API Word CIR::pop()
{
    Word top = stack.back();
    stack.pop_back();
    return top;
}

CIR_INLINE CIR_API void CIR::push(const Word& value)
{
    stack.push_back(value);
}

CIR_INLINE CIR_API void CIR::move(const Word& w, uint16_t i)
{
    registers[i] = w;
    registers[i].set_flag(WordFlag::Register);
}

CIR_INLINE CIR_API Word& CIR::getr(uint16_t i)
{
    return registers[i];
}

CIR_INLINE CIR_API Word& CIR::gets()
{
    return stack.emplace_back();
}

// Get Operand
CIR_INLINE Word& CIR::go(Word& w)
{
    if (w.has_flag(WordFlag::Register))
    {
        return getr(w.as_int());
    }
    else
    {
        return w;
    }
}

// Uniformed Operands Syntax (dest, reg/imm, imm/reg)
CIR_API void CIR::execute_op(Function& fn, Op op)
{
    switch (op.type)
    {
    // (imm/reg, reg)
    case OpType::Mov:
        {
            move(go(op.args[0]), op.args[1].as_int());
        }
        break;

    // (imm/reg)
    case OpType::Push:
        {
            push(go(op.args[0]));
        }
        break;


    // (reg)
    case OpType::Pop:
        {
            op.args[0].expect_flag(WordFlag::Register);
            move(pop(), static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    // (dest, imm/reg, imm/reg)
    case OpType::Add:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a + b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    // (dest, imm/reg, imm/reg)
    case OpType::Sub:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a - b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    // (dest, imm/reg, imm/reg)
    case OpType::Mul:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a * b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    // (dest, imm/reg, imm/reg)
    case OpType::Div:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a / b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Mod:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a % b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::And:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a & b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Or:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a | b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    // TODO: this and following need implementation
    case OpType::Xor:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a ^ b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Not:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            a.expect(WordType::Integer);
            move(Word::from_int(~a.as_int()), static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Shl:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a << b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Shr:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& a = go(op.args[1]);
            Word& b = go(op.args[2]);
            move(a >> b, static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Cmp:
        {
            Word& a = go(op.args[0]);
            Word& b = go(op.args[1]);
            cmp_flag = a == b;
        }
        break;

    case OpType::Jmp:
        {
            Word& a = go(op.args[0]);
            a.expect(WordType::Integer);
            fn.co = a.as_int();
        }
        break;

    case OpType::Je:
        {
            if (cmp_flag)
            {
                Word& a = go(op.args[0]);
                a.expect(WordType::Integer);
                fn.co = a.as_int();
            }
        }
        break;

    case OpType::Jne:
        {
            if (!cmp_flag)
            {
                Word& a = go(op.args[0]);
                a.expect(WordType::Integer);
                fn.co = a.as_int();
            }
        }
        break;

    case OpType::Gt:
        {
            Word& a = go(op.args[0]);
            Word& b = go(op.args[1]);

            cmp_flag = (a > b);
        }
        break;

    case OpType::Lt:
        {
            Word& a = go(op.args[0]);
            Word& b = go(op.args[1]);

            cmp_flag = (a < b);
        }
        break;

    case OpType::Gte:
        {
            Word& a = go(op.args[0]);
            Word& b = go(op.args[1]);

            cmp_flag = (a >= b);
        }
        break;

    case OpType::Lte:
        {
            Word& a = go(op.args[0]);
            Word& b = go(op.args[1]);

            cmp_flag = (a <= b);
        }
        break;

    case OpType::Inc:
        {
            op.args[0].expect_flag(WordFlag::Register);
            uint16_t idx = static_cast<uint16_t>(op.args[0].as_int());
            Word incremented = ++registers[idx];
            move(incremented, idx);
        }
        break;

    case OpType::Dec:
        {
            op.args[0].expect_flag(WordFlag::Register);
            uint16_t idx = static_cast<uint16_t>(op.args[0].as_int());
            Word decremented = --registers[idx];
            move(decremented, idx);
        }
        break;

    case OpType::Neg:
        {
            op.args[0].expect_flag(WordFlag::Register);
            uint16_t idx = static_cast<uint16_t>(op.args[0].as_int());
            Word& a = getr(idx);
            if (a.type == WordType::Integer)
            {
                move(Word::from_int(-a.as_int()), idx);
            }
            else if (a.type == WordType::Float)
            {
                move(Word::from_float(-a.as_float()), idx);
            }
            else
            {
                throw std::runtime_error("Invalid type for negation");
            }
        }
        break;

    case OpType::Cast:
        {
            op.args[1].expect_flag(WordFlag::Register);
            uint16_t dest_idx = static_cast<uint16_t>(op.args[1].as_int());
            Word& dest = getr(dest_idx);

            Word& target_type = go(op.args[0]);
            target_type.expect_flag(WordFlag::String);

            const char* type_str = static_cast<const char*>(target_type.as_ptr());

            if (dest.type == WordType::Integer)
            {
                if (strcmp(type_str, "float") == 0)
                {
                    move(Word::from_float(static_cast<double>(dest.as_int())), dest_idx);
                }
                else if (strcmp(type_str, "ptr") == 0)
                {
                    move(Word::from_ptr(reinterpret_cast<void*>(dest.as_int())), dest_idx);
                }
                else if (strcmp(type_str, "int") != 0)
                {
                    throw std::runtime_error("Invalid cast from int to " + std::string(type_str));
                }
            }
            else if (dest.type == WordType::Float)
            {
                if (strcmp(type_str, "int") == 0)
                {
                    move(Word::from_int(static_cast<int64_t>(dest.as_float())), dest_idx);
                }
                else if (strcmp(type_str, "float") != 0)
                {
                    throw std::runtime_error("Invalid cast from float to " + std::string(type_str));
                }
            }
            else if (dest.type == WordType::Pointer)
            {
                if (strcmp(type_str, "int") == 0)
                {
                    move(Word::from_int(reinterpret_cast<int64_t>(dest.as_ptr())), dest_idx);
                }
                else if (strcmp(type_str, "ptr") != 0)
                {
                    throw std::runtime_error("Invalid cast from ptr to " + std::string(type_str));
                }
            }
            else
            {
                throw std::runtime_error("Unsupported source type for cast");
            }
        }
        break;


    case OpType::Halt: program.state.running = false;
        break;

    case OpType::Nop: break;

    case OpType::Call:
        {
            CallFrame cf = {program.state.cf, fn.co + 1};
            program.state.call_stack.push_back(cf);
            program.state.cf = std::string((const char*)op.args[0].as_ptr());

            if (!program.functions.contains(program.state.cf))
            {
                throw std::runtime_error("Function not found: " + program.state.cf);
            }
            program.functions[program.state.cf].co = 0;
        }
        return;

    case OpType::CallExtern:
        {
            Word& fn_name_w = go(op.args[0]);
            fn_name_w.expect_flag(WordFlag::String);

            const char* fn_name_cstr = static_cast<const char*>(fn_name_w.as_ptr());
            if (fn_name_cstr == nullptr)
            {
                throw std::runtime_error("CallExtern: null function name");
            }

            std::string fn_name(fn_name_cstr);

            auto it = extern_functions.find(fn_name);
            if (it == extern_functions.end())
            {
                throw std::runtime_error("External function not found: " + fn_name);
            }

            it->second(*this);
        }
        break;

    case OpType::Ret:
        {
            if (program.state.call_stack.empty())
            {
                program.state.running = false;
                return;
            }

            CallFrame cf = program.state.call_stack.back();
            program.state.call_stack.pop_back();

            program.state.cf = cf.name;
            program.functions[program.state.cf].co = cf.co;
        }
        return;

    // (dest_reg, address, size)
    case OpType::Load:
        {
            op.args[0].expect_flag(WordFlag::Register);
            uint16_t dest_idx = static_cast<uint16_t>(op.args[0].as_int());

            Word& addr = go(op.args[1]);
            addr.expect(WordType::Pointer);

            Word& size = go(op.args[2]);
            size.expect(WordType::Integer);

            void* ptr = addr.as_ptr();
            if (ptr == nullptr)
            {
                throw std::runtime_error("Load: null pointer dereference");
            }

            int64_t byte_size = size.as_int();

            switch (byte_size)
            {
            case 1:
                move(Word::from_int(*static_cast<uint8_t*>(ptr)), dest_idx);
                break;
            case 2:
                move(Word::from_int(*static_cast<uint16_t*>(ptr)), dest_idx);
                break;
            case 4:
                move(Word::from_int(*static_cast<uint32_t*>(ptr)), dest_idx);
                break;
            case 8:
                {
                    Word w;
                    std::memcpy(&w.data, ptr, 8);
                    w.type = WordType::Integer;
                    move(w, dest_idx);
                }
                break;
            default:
                throw std::runtime_error("Load: unsupported size " + std::to_string(byte_size));
            }
        }
        break;

    // (ptr, value, size)
    case OpType::Store:
        {
            Word& addr = go(op.args[0]);
            addr.expect(WordType::Pointer);

            Word& value = go(op.args[1]);

            Word& size = go(op.args[2]);
            size.expect(WordType::Integer);

            void* ptr = addr.as_ptr();
            if (ptr == nullptr)
            {
                throw std::runtime_error("Store: null pointer dereference");
            }

            int64_t byte_size = size.as_int();

            switch (byte_size)
            {
            case 1:
                *static_cast<uint8_t*>(ptr) = static_cast<uint8_t>(value.as_int());
                break;
            case 2:
                *static_cast<uint16_t*>(ptr) = static_cast<uint16_t>(value.as_int());
                break;
            case 4:
                if (value.type == WordType::Float)
                {
                    *static_cast<float*>(ptr) = static_cast<float>(value.as_float());
                }
                else
                {
                    *static_cast<uint32_t*>(ptr) = static_cast<uint32_t>(value.as_int());
                }
                break;
            case 8:
                std::memcpy(ptr, &value.data, 8);
                break;
            default:
                throw std::runtime_error("Store: unsupported size " + std::to_string(byte_size));
            }
        }
        break;

    case OpType::Alloc:
        {
            op.args[0].expect_flag(WordFlag::Register);
            Word& x = go(op.args[1]);
            x.expect(WordType::Integer);
            move(Word::from_ptr(heap.allocate(x.as_int())), static_cast<uint16_t>(op.args[0].as_int()));
        }
        break;

    case OpType::Free:
        {
            heap.deallocate(getr(op.args[0].as_int()).as_ptr());
        }
        break;

    case OpType::Lea:
        {
            op.args[0].expect_flag(WordFlag::Register);
            uint16_t dest_idx = static_cast<uint16_t>(op.args[0].as_int());

            Word& base = go(op.args[1]);
            Word& offset = go(op.args[2]);

            offset.expect(WordType::Integer);

            void* address = nullptr;

            if (base.type == WordType::Pointer)
            {
                address = static_cast<char*>(base.as_ptr()) + offset.as_int();
            }
            else if (base.type == WordType::Integer)
            {
                address = reinterpret_cast<void*>(base.as_int() + offset.as_int());
            }
            else
            {
                throw std::runtime_error("lea: base must be pointer or integer");
            }

            move(Word::from_ptr(address), dest_idx);
        }
        break;

    default: assert(0 && "wtf, this dont should happen.");
    }
}

CIR_API void CIR::execute_function(const std::string& name)
{
    program.state.cf = name;
    program.state.running = true;

    if (!program.functions.contains(name))
    {
        throw std::runtime_error("Function not found: " + name);
    }

    program.functions[name].co = 0;

    while (program.state.running)
    {
        Function& fn = program.functions[program.state.cf];

        if (fn.co >= fn.ops.size())
        {
            if (program.state.call_stack.empty())
            {
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

CIR_INLINE CIR_API void CIR::check_externs()
{
    for (const auto& req : program.required_externs)
    {
        if (!extern_functions.contains(req))
        {
            throw std::runtime_error("Missing required external function: " + req);
        }
    }
}

CIR_API void CIR::execute_program()
{
    check_externs();
    execute_function("main");
}

CIR_API std::vector<uint8_t> CIR::to_bytecode()
{
    std::vector<uint8_t> bytes;

    std::unordered_map<std::string, uint32_t> string_table;
    std::vector<std::string> string_list;
    uint32_t string_index = 0;

    auto add_string = [&](const char* str) -> uint32_t
    {
        if (!str) return UINT32_MAX;
        std::string s(str);
        auto it = string_table.find(s);
        if (it != string_table.end())
        {
            return it->second;
        }
        string_table[s] = string_index;
        string_list.push_back(s);
        return string_index++;
    };

    for (const auto& [name, func] : program.functions)
    {
        add_string(name.c_str());

        for (const auto& op : func.ops)
        {
            for (size_t i = 0; i < Config::OpArgCount; i++)
            {
                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer)
                {
                    add_string(static_cast<const char*>(op.args[i].data.p));
                }
            }
        }
    }

    for (const auto& req : program.required_externs)
    {
        add_string(req.c_str());
    }

    uint32_t string_count = string_list.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&string_count),
                 reinterpret_cast<uint8_t*>(&string_count) + sizeof(string_count));

    for (const auto& str : string_list)
    {
        uint32_t str_len = str.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&str_len),
                     reinterpret_cast<uint8_t*>(&str_len) + sizeof(str_len));
        bytes.insert(bytes.end(), str.begin(), str.end());
        bytes.push_back(0);
    }

    uint32_t req_count = program.required_externs.size();
    bytes.insert(bytes.end(),
                 reinterpret_cast<uint8_t*>(&req_count),
                 reinterpret_cast<uint8_t*>(&req_count) + sizeof(req_count));

    for (const auto& req : program.required_externs)
    {
        uint32_t str_idx = string_table[req];
        bytes.insert(bytes.end(),
                     reinterpret_cast<uint8_t*>(&str_idx),
                     reinterpret_cast<uint8_t*>(&str_idx) + sizeof(str_idx));
    }

    uint32_t func_count = program.functions.size();
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&func_count),
                 reinterpret_cast<uint8_t*>(&func_count) + sizeof(func_count));

    for (const auto& [name, func] : program.functions)
    {
        uint32_t name_idx = string_table[name];
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&name_idx),
                     reinterpret_cast<uint8_t*>(&name_idx) + sizeof(name_idx));

        uint32_t op_count = func.ops.size();
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&op_count),
                     reinterpret_cast<uint8_t*>(&op_count) + sizeof(op_count));

        for (const auto& op : func.ops)
        {
            bytes.push_back(static_cast<uint8_t>(op.type));

            for (size_t i = 0; i < Config::OpArgCount; i++)
            {
                bytes.push_back(static_cast<uint8_t>(op.args[i].type));
                bytes.push_back(op.args[i].flags);

                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer)
                {
                    const char* str = static_cast<const char*>(op.args[i].data.p);
                    uint32_t str_idx = str ? string_table[std::string(str)] : UINT32_MAX;

                    bytes.insert(bytes.end(),
                                 reinterpret_cast<uint8_t*>(&str_idx),
                                 reinterpret_cast<uint8_t*>(&str_idx) + sizeof(str_idx));
                }
                else
                {
                    bytes.insert(bytes.end(),
                                 reinterpret_cast<const uint8_t*>(&op.args[i].data),
                                 reinterpret_cast<const uint8_t*>(&op.args[i].data) + sizeof(op.args[i].data));
                }
            }
        }
    }
    return bytes;
}

CIR_API void CIR::from_bytecode(const std::vector<uint8_t>& bytes)
{
    size_t offset = 0;
    program = Program{};

    if (bytes.size() < sizeof(uint32_t))
    {
        throw std::runtime_error("Bytecode too short: cannot read string count");
    }

    uint32_t string_count;
    std::memcpy(&string_count, &bytes[offset], sizeof(string_count));
    offset += sizeof(string_count);

    std::vector<std::string> string_table(string_count);

    for (uint32_t s = 0; s < string_count; s++)
    {
        if (offset + sizeof(uint32_t) > bytes.size())
        {
            throw std::runtime_error("Bytecode truncated: cannot read string length");
        }

        uint32_t str_len;
        std::memcpy(&str_len, &bytes[offset], sizeof(str_len));
        offset += sizeof(str_len);

        if (offset + str_len + 1 > bytes.size())
        {
            throw std::runtime_error("Bytecode truncated: cannot read string data");
        }

        string_table[s] = std::string(reinterpret_cast<const char*>(&bytes[offset]), str_len);
        offset += str_len + 1;
    }

    if (offset + sizeof(uint32_t) > bytes.size())
    {
        throw std::runtime_error("Bytecode truncated: cannot read required_externs count");
    }

    uint32_t req_count;
    std::memcpy(&req_count, &bytes[offset], sizeof(req_count));
    offset += sizeof(req_count);

    program.required_externs.clear();
    for (uint32_t i = 0; i < req_count; i++)
    {
        if (offset + sizeof(uint32_t) > bytes.size())
        {
            throw std::runtime_error("Bytecode truncated: cannot read required_externs string index");
        }

        uint32_t str_idx;
        std::memcpy(&str_idx, &bytes[offset], sizeof(str_idx));
        offset += sizeof(str_idx);

        if (str_idx >= string_table.size())
        {
            throw std::runtime_error("Invalid string table index for required_extern");
        }

        program.required_externs.push_back(string_table[str_idx]);
    }

    if (offset + sizeof(uint32_t) > bytes.size())
    {
        throw std::runtime_error("Bytecode too short: cannot read function count");
    }

    uint32_t func_count;
    std::memcpy(&func_count, &bytes[offset], sizeof(func_count));
    offset += sizeof(func_count);

    for (uint32_t f = 0; f < func_count; f++)
    {
        if (offset + sizeof(uint32_t) > bytes.size())
        {
            throw std::runtime_error("Bytecode truncated: cannot read function name index");
        }

        uint32_t name_idx;
        std::memcpy(&name_idx, &bytes[offset], sizeof(name_idx));
        offset += sizeof(name_idx);

        if (name_idx >= string_table.size())
        {
            throw std::runtime_error("Invalid string table index for function name");
        }

        const std::string& func_name = string_table[name_idx];

        Function func;

        if (offset + sizeof(uint32_t) > bytes.size())
        {
            throw std::runtime_error("Bytecode truncated: cannot read op count");
        }

        uint32_t op_count;
        std::memcpy(&op_count, &bytes[offset], sizeof(op_count));
        offset += sizeof(op_count);

        for (uint32_t o = 0; o < op_count; o++)
        {
            if (offset + 1 > bytes.size())
            {
                throw std::runtime_error("Bytecode truncated: cannot read op type");
            }

            Op op;
            op.type = static_cast<OpType>(bytes[offset++]);

            for (size_t i = 0; i < Config::OpArgCount; i++)
            {
                if (offset + 2 > bytes.size())
                {
                    throw std::runtime_error("Bytecode truncated: cannot read op argument type and flags");
                }

                op.args[i].type = static_cast<WordType>(bytes[offset++]);
                op.args[i].flags = bytes[offset++];

                if (op.args[i].has_flag(WordFlag::String) && op.args[i].type == WordType::Pointer)
                {
                    if (offset + sizeof(uint32_t) > bytes.size())
                    {
                        throw std::runtime_error("Bytecode truncated: cannot read string index");
                    }

                    uint32_t str_idx;
                    std::memcpy(&str_idx, &bytes[offset], sizeof(str_idx));
                    offset += sizeof(str_idx);

                    if (str_idx == UINT32_MAX)
                    {
                        op.args[i].data.p = nullptr;
                    }
                    else
                    {
                        if (str_idx >= string_table.size())
                        {
                            throw std::runtime_error("Invalid string table index");
                        }

                        const std::string& str = string_table[str_idx];
                        char* str_copy = new char[str.size() + 1];
                        std::strcpy(str_copy, str.c_str());

                        op.args[i].data.p = str_copy;
                        op.args[i].set_flag(WordFlag::OwnsMemory);
                    }
                }
                else
                {
                    if (offset + sizeof(op.args[i].data) > bytes.size())
                    {
                        throw std::runtime_error("Bytecode truncated: cannot read op argument data");
                    }

                    std::memcpy(&op.args[i].data, &bytes[offset], sizeof(op.args[i].data));
                    offset += sizeof(op.args[i].data);
                }
            }

            func.ops.push_back(op);
        }

        program.functions[func_name] = func;
    }
}

CIR_API void CIR::load_program(Program p)
{
    program = std::move(p);
}

CIR_API Program& CIR::get_program()
{
    return program;
}

CIR_API void CIR::set_extern_fn(std::string n, CIR_ExternFn f)
{
    extern_functions[n] = f;
}

CIR_API std::vector<Word>& CIR::get_stack()
{
    return stack;
}


#endif
