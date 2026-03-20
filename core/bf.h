#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stack>
#include <stdexcept>
#include "cir.h"

// Brainfuck to CIR compiler.
//
// Register assignments:
//   r0 - I/O register (argument/return for std.putchar / std.getchar)
//   r1 - data pointer (address of the current tape cell)
//   r2 - temporary cell value
//   r3 - tape base pointer (saved for free on exit)
//
// Jump target encoding: stored as (instruction_index - 1) because
// execute_function does fn.co = target; fn.co++ after each op.
class BrainfuckCompiler
{
    static Op make_op(OpType type, Word a0, Word a1, Word a2)
    {
        Op op;
        op.type = type;
        op.args[0] = std::move(a0);
        op.args[1] = std::move(a1);
        op.args[2] = std::move(a2);
        return op;
    }

    static Op make_op(OpType type, Word a0, Word a1)
    {
        return make_op(type, std::move(a0), std::move(a1), Word::from_null());
    }

    static Op make_op(OpType type, Word a0)
    {
        return make_op(type, std::move(a0), Word::from_null(), Word::from_null());
    }

    static Op make_op(OpType type)
    {
        return make_op(type, Word::from_null(), Word::from_null(), Word::from_null());
    }

    // load r2, r1, $1  ->  r2 = *r1  (1-byte cell)
    static void emit_load_cell(Function& fn)
    {
        fn.ops.push_back(make_op(OpType::Load,
            Word::from_reg(2),
            Word::from_reg(1),
            Word::from_int(1)
        ));
    }

    // store r1, r2, $1  ->  *r1 = r2  (1-byte cell)
    static void emit_store_cell(Function& fn)
    {
        fn.ops.push_back(make_op(OpType::Store,
            Word::from_reg(1),
            Word::from_reg(2),
            Word::from_int(1)
        ));
    }

    // cmp r2, $0
    static void emit_cmp_zero(Function& fn)
    {
        fn.ops.push_back(make_op(OpType::Cmp,
            Word::from_reg(2),
            Word::from_int(0)
        ));
    }

public:
    // Compile a Brainfuck source string into a CIR Program.
    Program compile(const std::string& source)
    {
        Program prog;
        Function main_func;

        // alloc r3, $30000  ; allocate zero-initialized tape
        main_func.ops.push_back(make_op(OpType::Alloc,
            Word::from_reg(3),
            Word::from_int(30000)
        ));

        // mov r3, r1  ; data pointer starts at tape base
        main_func.ops.push_back(make_op(OpType::Mov,
            Word::from_reg(3),
            Word::from_reg(1)
        ));

        // loop_stack holds the index of the `je` instruction emitted for each `[`
        std::stack<size_t> loop_stack;

        for (char c : source)
        {
            switch (c)
            {
            case '>':
                // add r1, r1, $1
                main_func.ops.push_back(make_op(OpType::Add,
                    Word::from_reg(1),
                    Word::from_reg(1),
                    Word::from_int(1)
                ));
                break;

            case '<':
                // sub r1, r1, $1
                main_func.ops.push_back(make_op(OpType::Sub,
                    Word::from_reg(1),
                    Word::from_reg(1),
                    Word::from_int(1)
                ));
                break;

            case '+':
                // load r2, r1, $1 / add r2, r2, $1 / store r1, r2, $1
                emit_load_cell(main_func);
                main_func.ops.push_back(make_op(OpType::Add,
                    Word::from_reg(2),
                    Word::from_reg(2),
                    Word::from_int(1)
                ));
                emit_store_cell(main_func);
                break;

            case '-':
                // load r2, r1, $1 / sub r2, r2, $1 / store r1, r2, $1
                emit_load_cell(main_func);
                main_func.ops.push_back(make_op(OpType::Sub,
                    Word::from_reg(2),
                    Word::from_reg(2),
                    Word::from_int(1)
                ));
                emit_store_cell(main_func);
                break;

            case '.':
                // load r2, r1, $1 / mov r2, r0 / callx std.putchar
                emit_load_cell(main_func);
                main_func.ops.push_back(make_op(OpType::Mov,
                    Word::from_reg(2),
                    Word::from_reg(0)
                ));
                main_func.ops.push_back(make_op(OpType::CallExtern,
                    Word::from_string_owned("std.putchar")
                ));
                break;

            case ',':
                // callx std.getchar / store r1, r0, $1
                main_func.ops.push_back(make_op(OpType::CallExtern,
                    Word::from_string_owned("std.getchar")
                ));
                main_func.ops.push_back(make_op(OpType::Store,
                    Word::from_reg(1),
                    Word::from_reg(0),
                    Word::from_int(1)
                ));
                break;

            case '[':
                {
                    // emit: load r2, r1, $1
                    // emit: cmp r2, $0
                    // emit: je PLACEHOLDER   <- index saved in loop_stack
                    emit_load_cell(main_func);
                    emit_cmp_zero(main_func);
                    size_t je_index = main_func.ops.size();
                    loop_stack.push(je_index);
                    main_func.ops.push_back(make_op(OpType::Je,
                        Word::from_int(-1) // placeholder, patched when `]` is seen
                    ));
                }
                break;

            case ']':
                {
                    if (loop_stack.empty())
                    {
                        throw std::runtime_error("Unmatched ']' in Brainfuck source");
                    }
                    size_t je_index = loop_stack.top();
                    loop_stack.pop();

                    // The `[` check starts at (je_index - 2): load, cmp, je
                    // jne target: we want to jump back to the load at (je_index - 2)
                    // Stored value = target - 1 = (je_index - 2) - 1 = je_index - 3
                    emit_load_cell(main_func);
                    emit_cmp_zero(main_func);
                    main_func.ops.push_back(make_op(OpType::Jne,
                        Word::from_int(static_cast<int64_t>(je_index) - 3)
                    ));

                    // Patch `[`'s je to jump to loop_end (current size)
                    // Stored value = loop_end - 1
                    size_t loop_end = main_func.ops.size();
                    main_func.ops[je_index].args[0] =
                        Word::from_int(static_cast<int64_t>(loop_end) - 1);
                }
                break;

            default:
                break; // ignore non-BF characters (comments)
            }
        }

        if (!loop_stack.empty())
        {
            throw std::runtime_error("Unmatched '[' in Brainfuck source");
        }

        // free r3  ; release tape
        main_func.ops.push_back(make_op(OpType::Free, Word::from_reg(3)));

        // halt
        main_func.ops.push_back(make_op(OpType::Halt));

        prog.functions["main"] = std::move(main_func);
        prog.required_externs = {"std.putchar", "std.getchar"};
        return prog;
    }

    // Compile a Brainfuck source file into a CIR Program.
    Program compile_file(const std::string& filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        std::ostringstream oss;
        oss << file.rdbuf();
        return compile(oss.str());
    }
};
