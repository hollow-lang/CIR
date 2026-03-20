#define CIR_IMPLEMENTATION
#include <iostream>
#include <filesystem>
#include <chrono>

#include "../../core/bf.h"
#include "../../core/std.h"

namespace fs = std::filesystem;

static void print_help(const char* prog)
{
    std::cout << "Usage: " << prog << " <file.bf> [options]\n\n"
              << "Compile and run a Brainfuck program on the CIR virtual machine.\n\n"
              << "Options:\n"
              << "  -b, --benchmark   Show execution time\n"
              << "  -q, --quiet       Suppress info messages\n"
              << "  -h, --help        Show this help message\n"
              << "  --version         Show version information\n";
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        print_help(argv[0]);
        return 0;
    }

    std::string input_file;
    bool benchmark = false;
    bool quiet = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);

        if (arg == "-h" || arg == "--help")
        {
            print_help(argv[0]);
            return 0;
        }
        else if (arg == "--version")
        {
            std::cout << "bfcas (CIR Brainfuck) v" << Config::VERSION << "\n"
                      << "Copyright (c) 2025, " << Config::AUTHORS << "\n";
            return 0;
        }
        else if (arg == "-b" || arg == "--benchmark")
        {
            benchmark = true;
        }
        else if (arg == "-q" || arg == "--quiet")
        {
            quiet = true;
        }
        else if (arg[0] == '-')
        {
            std::cerr << "[ERROR] Unknown option: " << arg << "\n";
            return 1;
        }
        else
        {
            if (!input_file.empty())
            {
                std::cerr << "[ERROR] Multiple input files specified\n";
                return 1;
            }
            input_file = arg;
        }
    }

    if (input_file.empty())
    {
        std::cerr << "[ERROR] No input file specified\n";
        return 1;
    }

    if (!fs::exists(input_file))
    {
        std::cerr << "[ERROR] File not found: " << input_file << "\n";
        return 1;
    }

    try
    {
        if (!quiet) std::cout << "[INFO] Compiling: " << input_file << "\n";

        BrainfuckCompiler compiler;
        Program prog = compiler.compile_file(input_file);

        if (!quiet) std::cout << "[INFO] Running...\n";

        CIR cir;
        cir_std::init_std(cir);
        cir.load_program(std::move(prog));

        auto start = std::chrono::high_resolution_clock::now();
        cir.execute_program();
        auto end = std::chrono::high_resolution_clock::now();

        if (!quiet) std::cout << "\n[SUCCESS] Done\n";

        if (benchmark)
        {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            std::cout << "Execution time: " << us << " μs\n";
        }

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
