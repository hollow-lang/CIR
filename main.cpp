#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include "core/cir.h"
#include "core/asm.h"

namespace fs = std::filesystem;

struct CliConfig {
    std::string program_name;
    std::string input_file;
    std::string output_file;
    bool verbose = false;
    bool skip_compile = false;
    bool skip_run = false;
    bool show_stack = false;
    bool show_registers = false;
    bool benchmark = false;
    bool disassemble = false;
    int log_level = 1;
};

void print_word(const Word& w) {
    switch (w.type) {
        case WordType::Integer:
            std::cout << w.as_int();
            break;
        case WordType::Float:
            std::cout << std::fixed << std::setprecision(2) << w.as_float();
            break;
        case WordType::Pointer:
            if (w.has_flag(WordFlag::String)) {
                std::cout << static_cast<char*>(w.as_ptr());
            } else {
                std::cout << w.as_ptr();
            }
            break;
        case WordType::Boolean:
            std::cout << (w.as_bool() ? "true" : "false");
            break;
        case WordType::Null:
            std::cout << "null";
            break;
    }
}

namespace cir_std {
    void print(CIR &cir) {
        print_word(cir.getr(0));
        std::cout << std::endl;
    }
}

class Logger {
private:
    int level;

public:
    explicit Logger(int log_level) : level(log_level) {}

    void info(const std::string& msg) {
        if (level >= 1) {
            std::cout << "[INFO] " << msg << std::endl;
        }
    }

    void debug(const std::string& msg) {
        if (level >= 2) {
            std::cout << "[DEBUG] " << msg << std::endl;
        }
    }

    void error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << std::endl;
    }

    void success(const std::string& msg) {
        if (level >= 1) {
            std::cout << "[SUCCESS] " << msg << std::endl;
        }
    }
};

class CliTool {
private:
    CliConfig config;
    Logger logger;
    CIR cir;

    void register_stdlib() {
        cir.set_extern_fn("print", cir_std::print);
    }



    void print_stack() {
        std::cout << "Stack Contents: " << std::endl;

        auto& stack = cir.get_stack();
        if (stack.empty()) {
            std::cout << "(empty)" << std::endl;
        } else {
            for (size_t i = 0; i < stack.size(); ++i) {
                std::cout << "[" << std::setw(2) << i << "] ";
                print_word(stack[i]);
                std::cout << std::string(20 - std::min(20, (int)std::to_string(i).length()), ' ') << std::endl;
            }
        }
    }

    void print_registers() {
        std::cout << "Register contents:" << std::endl;

        for (int i = 0; i < std::min(8, Config::REGISTER_COUNT); ++i) {
            std::cout << "  r" << i << ": ";
            print_word(cir.getr(i));
            std::cout << std::string(23, ' ')  << std::endl;
        }

    }

    bool validate_input_file() {
        if (!fs::exists(config.input_file)) {
            logger.error("Input file does not exist: " + config.input_file);
            return false;
        }

        if (!fs::is_regular_file(config.input_file)) {
            logger.error("Input path is not a file: " + config.input_file);
            return false;
        }

        return true;
    }

    bool compile() {
        logger.info("Compiling: " + config.input_file);

        try {
            Assembler assembler;
            if (!config.verbose) {
                assembler.show_better_practice = false;
            }
            assembler.assemble_file(config.input_file);

            logger.debug("Assembly completed, generating bytecode");

            assembler.write_bytecode(config.output_file);

            auto file_size = fs::file_size(config.output_file);
            logger.success("Bytecode written to: " + config.output_file +
                          " (" + std::to_string(file_size) + " bytes)");

            cir.load_program(assembler.get_program());
            return true;

        } catch (const std::exception& e) {
            logger.error("Compilation failed: " + std::string(e.what()));
            return false;
        }
    }

    bool load_bytecode() {
        logger.info("Loading bytecode: " + config.output_file);

        try {
            std::ifstream f(config.output_file, std::ios::binary);
            if (!f) {
                logger.error("Cannot open bytecode file: " + config.output_file);
                return false;
            }

            std::vector<uint8_t> bytecode(
                (std::istreambuf_iterator<char>(f)),
                std::istreambuf_iterator<char>()
            );

            logger.debug("Loaded " + std::to_string(bytecode.size()) + " bytes");

            cir.from_bytecode(bytecode);
            logger.success("Bytecode loaded successfully");
            return true;

        } catch (const std::exception& e) {
            logger.error("Failed to load bytecode: " + std::string(e.what()));
            return false;
        }
    }

    bool execute() {
        logger.info("Executing program");

        try {
            auto start = std::chrono::high_resolution_clock::now();

            register_stdlib();
            cir.execute_program();

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            logger.success("Program executed successfully");
            if (config.benchmark) {
                std::cout << "\nExecution time: " << duration.count() << " μs" << std::endl;
            }

            if (config.show_stack) {
                print_stack();
            }

            if (config.show_registers) {
                print_registers();
            }

            return true;

        } catch (const std::exception& e) {
            logger.error("Execution failed: " + std::string(e.what()));
            return false;
        }
    }

public:
    explicit CliTool(const CliConfig& cfg) : config(cfg), logger(cfg.log_level) {}

    int run() {
        logger.debug("Starting CLI tool");

        if (!config.skip_compile) {
            if (!validate_input_file()) {
                return 1;
            }

            if (!compile()) {
                return 1;
            }
        } else {
            if (!load_bytecode()) {
                return 1;
            }
        }

        if (!config.skip_run) {
            if (!execute()) {
                return 1;
            }
        }

        return 0;
    }
};

class ArgParser {
private:
    CliConfig config;

    static void print_version() {
        std::cout << "CIR v" << Config::VERSION << std::endl;
        std::cout << "Copyright (c) 2025, " << Config::AUTHORS << std::endl;
    }

    void print_help() {
        std::cout << "Usage: " << config.program_name << " <input_file> [options]\n" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -o, --output <file>      Specify output bytecode file (default: program.bin)" << std::endl;
        std::cout << "  -c, --no-compile         Skip compilation, run existing bytecode" << std::endl;
        std::cout << "  -r, --no-run             Compile only, don't execute" << std::endl;
        std::cout << "  -v, --verbose            Enable verbose output" << std::endl;
        std::cout << "  -vv, --debug             Enable debug output" << std::endl;
        std::cout << "  -s, --show-stack         Display stack contents after execution" << std::endl;
        std::cout << "  -g, --show-registers     Display register contents after execution" << std::endl;
        std::cout << "  -b, --benchmark          Show execution time" << std::endl;
        std::cout << "  -q, --quiet              Suppress all non-error output" << std::endl;
        std::cout << "  -h, --help               Display this help message" << std::endl;
        std::cout << "  --version                Display version information" << std::endl;
        std::cout << "\nExamples:" << std::endl;
        std::cout << "  " << config.program_name << " program.asm" << std::endl;
        std::cout << "  " << config.program_name << " program.asm -o out.bin -v" << std::endl;
        std::cout << "  " << config.program_name << " -c -o program.bin --show-stack" << std::endl;
    }

public:
    CliConfig parse(int argc, char** argv) {
        if (argc < 1) {
            throw std::runtime_error("Invalid argument count");
        }

        config.program_name = argv[0];

        if (argc < 2) {
            print_help();
            exit(0);
        }

        std::vector<std::string> args(argv + 1, argv + argc);

        for (size_t i = 0; i < args.size(); ++i) {
            const auto& arg = args[i];

            if (arg == "-h" || arg == "--help") {
                print_help();
                exit(0);
            } else if (arg == "--version") {
                print_version();
                exit(0);
            } else if (arg == "-v" || arg == "--verbose") {
                config.log_level = 2;
                config.verbose = true;
            } else if (arg == "-vv" || arg == "--debug") {
                config.log_level = 3;
                config.verbose = true;
            } else if (arg == "-q" || arg == "--quiet") {
                config.log_level = 0;
            } else if (arg == "-c" || arg == "--no-compile") {
                config.skip_compile = true;
            } else if (arg == "-r" || arg == "--no-run") {
                config.skip_run = true;
            } else if (arg == "-s" || arg == "--show-stack") {
                config.show_stack = true;
            } else if (arg == "-g" || arg == "--show-registers") {
                config.show_registers = true;
            } else if (arg == "-b" || arg == "--benchmark") {
                config.benchmark = true;
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 >= args.size()) {
                    throw std::runtime_error("Missing value for " + arg);
                }
                config.output_file = args[++i];
            } else if (arg[0] == '-') {
                throw std::runtime_error("Unknown option: " + arg);
            } else {
                if (config.input_file.empty()) {
                    config.input_file = arg;
                } else {
                    throw std::runtime_error("Multiple input files specified");
                }
            }
        }

        if (config.input_file.empty() && !config.skip_compile) {
            throw std::runtime_error("No input file specified");
        }

        if (config.output_file.empty()) {
            if (config.skip_compile) {
                config.output_file = "program.bin";
            } else {
                fs::path input_path(config.input_file);
                config.output_file = input_path.stem().string() + ".bin";
            }
        }

        return config;
    }
};

int main(int argc, char* argv[]) {
    try {
        ArgParser parser;
        CliConfig config = parser.parse(argc, argv);

        CliTool tool(config);
        return tool.run();

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}