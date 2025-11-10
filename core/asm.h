#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <algorithm>
#include "cir.h"
#include "helpers/scalc.h"

class Assembler {
public:
    bool show_better_practice = true;
private:
    std::unordered_map<std::string, OpType> opcode_map;
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> labels;
    std::unordered_set<std::string> forward_label_refs;
    Program program;
    std::string current_function;
    size_t line_number = 0;

    CTEE ctee;


    void init_opcode_map() {
        opcode_map["mov"] = OpType::Mov;
        opcode_map["push"] = OpType::Push;
        opcode_map["pushr"] = OpType::PushReg;
        opcode_map["pop"] = OpType::Pop;
        opcode_map["iadd"] = OpType::IAdd;
        opcode_map["isub"] = OpType::ISub;
        opcode_map["imul"] = OpType::IMul;
        opcode_map["idiv"] = OpType::IDiv;
        opcode_map["imod"] = OpType::IMod;
        opcode_map["and"] = OpType::And;
        opcode_map["or"] = OpType::Or;
        opcode_map["xor"] = OpType::Xor;
        opcode_map["not"] = OpType::Not;
        opcode_map["shl"] = OpType::Shl;
        opcode_map["shr"] = OpType::Shr;
        opcode_map["icmp"] = OpType::ICmp;
        opcode_map["jmp"] = OpType::Jmp;
        opcode_map["je"] = OpType::Je;
        opcode_map["jne"] = OpType::Jne;
        opcode_map["jg"] = OpType::Jg;
        opcode_map["jl"] = OpType::Jl;
        opcode_map["jge"] = OpType::Jge;
        opcode_map["jle"] = OpType::Jle;
        opcode_map["call"] = OpType::Call;
        opcode_map["callx"] = OpType::CallExtern;
        opcode_map["ret"] = OpType::Ret;
        opcode_map["load"] = OpType::Load;
        opcode_map["store"] = OpType::Store;
        opcode_map["halt"] = OpType::Halt;
        opcode_map["nop"] = OpType::Nop;
        opcode_map["inc"] = OpType::Inc;
        opcode_map["dec"] = OpType::Dec;
        opcode_map["neg"] = OpType::Neg;
        opcode_map["fadd"] = OpType::FAdd;
        opcode_map["fsub"] = OpType::FSub;
        opcode_map["fmul"] = OpType::FMul;
        opcode_map["fdiv"] = OpType::FDiv;
        opcode_map["fcmp"] = OpType::FCmp;
        opcode_map["cast"] = OpType::Cast;
        opcode_map["local.get"] = OpType::LocalGet;
        opcode_map["local.set"] = OpType::LocalSet;
    }

    std::string trim(const std::string& str) {
        size_t start = 0;
        while (start < str.size() && std::isspace(str[start])) start++;

        size_t end = str.size();
        while (end > start && std::isspace(str[end - 1])) end--;

        return str.substr(start, end - start);
    }

    std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delim)) {
            token = trim(token);
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    bool looks_like_number(const std::string& str) {
        if (str.empty()) return false;

        size_t start = 0;
        if (str[0] == '-' || str[0] == '+') start = 1;
        if (start >= str.size()) return false;

        if (str.size() > start + 2 && str[start] == '0' &&
            (str[start + 1] == 'x' || str[start + 1] == 'X')) {
            for (size_t i = start + 2; i < str.size(); i++) {
                if (!std::isxdigit(str[i])) return false;
            }
            return true;
        }

        if (str.size() > start + 2 && str[start] == '0' &&
            (str[start + 1] == 'b' || str[start + 1] == 'B')) {
            for (size_t i = start + 2; i < str.size(); i++) {
                if (str[i] != '0' && str[i] != '1') return false;
            }
            return true;
        }

        bool has_dot = false;
        bool has_exp = false;
        for (size_t i = start; i < str.size(); i++) {
            if (str[i] == '.') {
                if (has_dot || has_exp) return false;
                has_dot = true;
            } else if (str[i] == 'e' || str[i] == 'E') {
                if (has_exp) return false;
                has_exp = true;
                if (i + 1 < str.size() && (str[i + 1] == '+' || str[i + 1] == '-')) {
                    i++;
                }
            } else if (!std::isdigit(str[i])) {
                return false;
            }
        }
        return true;
    }

    Word parse_operand(const std::string& operand, bool is_jump = false) {
        std::string op = trim(operand);

        // NOTE: returns double
        if (op.starts_with("comp(") && op.ends_with(")")) {
            std::string expr = op.substr(5, op.size() - 1);
            std::unordered_map<std::string, double> temp_ctx{
                labels[current_function].begin(), labels[current_function].end()
            };

            return Word::from_float(ctee.eval(expr, temp_ctx));
        }

        if (op[0] == '@') {
            std::string label = op.substr(1);
            if (labels[current_function].find(label) == labels[current_function].end()) {
                forward_label_refs.insert(current_function + ":" + label);
            }
            return Word::from_int(static_cast<int64_t>(labels[current_function][label]));
        }

        // explicit id "#name"
        if (op[0] == '#') {
            std::string id = op.substr(1);
            return Word::from_string_owned(id);
        }

        if (op[0] == 'r' && op.size() > 1 && std::isdigit(op[1])) {
            int reg_num = std::stoi(op.substr(1));
            if (reg_num < 0 || reg_num >= Config::REGISTER_COUNT) {
                throw std::runtime_error("Invalid register number r" + std::to_string(reg_num) +
                                       " (valid range: r0-r" + std::to_string(Config::REGISTER_COUNT - 1) + ")");
            }
            return Word::from_reg(reg_num);
        }

        if (op[0] == '"' && op.back() == '"') {
            std::string str = op.substr(1, op.size() - 2);
            std::string unescaped;
            for (size_t i = 0; i < str.size(); i++) {
                if (str[i] == '\\' && i + 1 < str.size()) {
                    switch (str[i + 1]) {
                        case 'n': unescaped += '\n'; i++; break;
                        case 't': unescaped += '\t'; i++; break;
                        case 'r': unescaped += '\r'; i++; break;
                        case '\\': unescaped += '\\'; i++; break;
                        case '"': unescaped += '"'; i++; break;
                        default: unescaped += str[i]; break;
                    }
                } else {
                    unescaped += str[i];
                }
            }
            return Word::from_string_owned(unescaped);
        }

        if (op == "true" || op == "TRUE") {
            return Word::from_bool(true);
        }
        if (op == "false" || op == "FALSE") {
            return Word::from_bool(false);
        }

        if (op == "null" || op == "NULL") {
            return Word::from_null();
        }

        if (op[0] == '$') {
            std::string num_str = op.substr(1);

            if (num_str.empty()) {
                throw std::runtime_error("Invalid numeric literal: empty value after '$'");
            }

            if (num_str.find('.') != std::string::npos ||
                num_str.find('e') != std::string::npos ||
                num_str.find('E') != std::string::npos) {
                try {
                    return Word::from_float(std::stod(num_str));
                } catch (...) {
                    throw std::runtime_error("Invalid float literal: $" + num_str);
                }
            }

            if (num_str.size() > 2 && num_str[0] == '0' && (num_str[1] == 'x' || num_str[1] == 'X')) {
                try {
                    return Word::from_int(std::stoll(num_str, nullptr, 16));
                } catch (...) {
                    throw std::runtime_error("Invalid hexadecimal literal: $" + num_str);
                }
            }

            if (num_str.size() > 2 && num_str[0] == '0' && (num_str[1] == 'b' || num_str[1] == 'B')) {
                try {
                    return Word::from_int(std::stoll(num_str.substr(2), nullptr, 2));
                } catch (...) {
                    throw std::runtime_error("Invalid binary literal: $" + num_str);
                }
            }

            if (num_str.size() > 1 && num_str[0] == '0' && std::isdigit(num_str[1])) {
                try {
                    return Word::from_int(std::stoll(num_str, nullptr, 8));
                } catch (...) {
                    throw std::runtime_error("Invalid octal literal: $" + num_str);
                }
            }

            try {
                return Word::from_int(std::stoll(num_str));
            } catch (...) {
                throw std::runtime_error("Invalid integer literal: $" + num_str);
            }
        }

        if (op.size() >= 3 && op[0] == '\'' && op.back() == '\'') {
            char c = op[1];
            if (op[1] == '\\' && op.size() >= 4) {
                switch (op[2]) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case 'r': c = '\r'; break;
                    case '0': c = '\0'; break;
                    case '\\': c = '\\'; break;
                    case '\'': c = '\''; break;
                    default: c = op[2]; break;
                }
            }
            return Word::from_int(static_cast<int64_t>(c));
        }

        if (looks_like_number(op)) {
            throw std::runtime_error("Numeric literal '" + op + "' must be prefixed with '$' (e.g., $" + op + ")");
        }

        if (show_better_practice) std::cerr << "Note (line " << line_number << "): Operand '" << op << "' is being treated as a plain string.\n"
          << "Mandatory prefixes:\n"
          << "  - Numbers must start with $ (e.g., $123, $0xFF, $0b101)\n"
          << "  - Labels must start with @ (e.g., @loop_start)\n"
          << "  - Registers must be r0-r" << (Config::REGISTER_COUNT - 1) << "\n"
          << "  - Strings:   \"text\"\n"
          << "Optional for readability:\n"
          << "  - IDs:       #name\n" << std::endl;

        return Word::from_string_owned(op);
    }

    void validate_instruction(const Op& op, const std::string& opcode) {
        switch (op.type) {
            case OpType::Halt:
            case OpType::Nop:
            case OpType::Ret:
                break;

            case OpType::Not:
            case OpType::Inc:
            case OpType::Dec:
            case OpType::Neg:
            case OpType::Push:
            case OpType::PushReg:
            case OpType::Pop:
            case OpType::Jmp:
            case OpType::Call:
            case OpType::CallExtern:
            case OpType::LocalGet:
                if (op.args[0].type == WordType::Null) {
                    throw std::runtime_error("Instruction '" + opcode + "' requires 1 operand");
                }
                break;

            case OpType::Mov:
            case OpType::IAdd:
            case OpType::ISub:
            case OpType::IMul:
            case OpType::IDiv:
            case OpType::IMod:
            case OpType::And:
            case OpType::Or:
            case OpType::Xor:
            case OpType::Shl:
            case OpType::Shr:
            case OpType::ICmp:
            case OpType::Je:
            case OpType::Jne:
            case OpType::Jg:
            case OpType::Jl:
            case OpType::Jge:
            case OpType::Jle:
            case OpType::Load:
            case OpType::Store:
            case OpType::FAdd:
            case OpType::FSub:
            case OpType::FMul:
            case OpType::FDiv:
            case OpType::FCmp:
            case OpType::Cast:
            case OpType::LocalSet:
                if (op.args[0].type == WordType::Null || op.args[1].type == WordType::Null) {
                    throw std::runtime_error("Instruction '" + opcode + "' requires 2 operands");
                }
                break;
        }
    }

    void assemble_line(const std::string& line, Function& func) {
        std::string cleaned = trim(line);

        size_t comment_pos = cleaned.find(';');
        if (comment_pos != std::string::npos) {
            cleaned = cleaned.substr(0, comment_pos);
            cleaned = trim(cleaned);
        }

        if (cleaned.empty()) return;

        if (cleaned.back() == ':') {
            std::string label = cleaned.substr(0, cleaned.size() - 1);
            label = trim(label);
            if (label[0] == '.') label = label.substr(1);

            if (labels[current_function].find(label) != labels[current_function].end()) {
                throw std::runtime_error("Duplicate label: " + label);
            }

            labels[current_function][label] = func.ops.size();
            return;
        }

        size_t first_space = cleaned.find(' ');
        std::string opcode = (first_space == std::string::npos) ? cleaned : cleaned.substr(0, first_space);
        std::string original_opcode = opcode;
        std::transform(opcode.begin(), opcode.end(), opcode.begin(), ::tolower);

        if (opcode_map.find(opcode) == opcode_map.end()) {
            throw std::runtime_error("Unknown opcode: " + original_opcode);
        }

        Op op;
        op.type = opcode_map[opcode];

        for (size_t i = 0; i < Config::OpArgCount; i++) {
            op.args[i] = Word::from_null();
        }

        bool is_jump = (op.type == OpType::Jmp || op.type == OpType::Je || op.type == OpType::Jne ||
                       op.type == OpType::Jg || op.type == OpType::Jl ||
                       op.type == OpType::Jge || op.type == OpType::Jle);

        if (first_space != std::string::npos) {
            std::string operands_str = cleaned.substr(first_space + 1);
            std::vector<std::string> operands = split(operands_str, ',');

            if (operands.size() > Config::OpArgCount) {
                throw std::runtime_error("Too many operands for instruction '" + opcode +
                                       "' (max " + std::to_string(Config::OpArgCount) + ")");
            }

            for (size_t i = 0; i < operands.size() && i < Config::OpArgCount; i++) {
                op.args[i] = parse_operand(operands[i], is_jump && i == 0);
            }
        }

        validate_instruction(op, opcode);
        func.ops.push_back(op);
    }

    void verify_labels() {
        for (const auto& ref : forward_label_refs) {
            size_t colon_pos = ref.find(':');
            std::string func_name = ref.substr(0, colon_pos);
            std::string label = ref.substr(colon_pos + 1);

            if (labels[func_name].find(label) == labels[func_name].end()) {
                throw std::runtime_error("Undefined label '" + label + "' in function '" + func_name + "'");
            }
        }
    }

    void verify_functions() {
        if (program.functions.empty()) {
            throw std::runtime_error("No functions defined in program");
        }

        if (program.functions.find("main") == program.functions.end()) {
            throw std::runtime_error("No 'main' function defined");
        }
    }

public:
    Assembler() {
        init_opcode_map();
    }

    void assemble_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::string line;
        Function* current_func = nullptr;
        line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
            std::string cleaned = trim(line);

            if (cleaned.empty() || cleaned[0] == ';' || cleaned[0] == '#') continue;

            if (cleaned.find(".fn ") == 0) {
                size_t name_start = 4;
                current_function = trim(cleaned.substr(name_start));

                if (current_function.empty()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": Function name cannot be empty");
                }

                if (program.functions.find(current_function) != program.functions.end()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                           ": Duplicate function definition: " + current_function);
                }

                program.functions[current_function] = Function();
                current_func = &program.functions[current_function];
                labels[current_function] = std::unordered_map<std::string, size_t>();
                continue;
            }

            if (cleaned == ".end") {
                if (current_func == nullptr) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                           ": .end without matching .fn");
                }
                current_func = nullptr;
                current_function.clear();
                continue;
            }

            if (current_func != nullptr) {
                try {
                    assemble_line(cleaned, *current_func);
                } catch (const std::exception& e) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": " + e.what());
                }
            } else {
                throw std::runtime_error("Line " + std::to_string(line_number) +
                                       ": Instruction outside function: " + cleaned);
            }
        }

        if (current_func != nullptr) {
            throw std::runtime_error("Missing .end for function: " + current_function);
        }

        file.close();

        verify_functions();
        verify_labels();
    }

    void assemble_string(const std::string& source) {
        std::istringstream stream(source);
        std::string line;
        Function* current_func = nullptr;
        line_number = 0;

        while (std::getline(stream, line)) {
            line_number++;
            std::string cleaned = trim(line);

            if (cleaned.empty() || cleaned[0] == ';' || cleaned[0] == '#') continue;

            if (cleaned.find(".fn ") == 0) {
                size_t name_start = 4;
                current_function = trim(cleaned.substr(name_start));

                if (current_function.empty()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": Function name cannot be empty");
                }

                if (program.functions.find(current_function) != program.functions.end()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                           ": Duplicate function definition: " + current_function);
                }

                program.functions[current_function] = Function();
                current_func = &program.functions[current_function];
                labels[current_function] = std::unordered_map<std::string, size_t>();
                continue;
            }

            if (cleaned == ".end") {
                if (current_func == nullptr) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                           ": .end without matching .fn");
                }
                current_func = nullptr;
                current_function.clear();
                continue;
            }

            if (current_func != nullptr) {
                try {
                    assemble_line(cleaned, *current_func);
                } catch (const std::exception& e) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": " + e.what());
                }
            }
        }

        if (current_func != nullptr) {
            throw std::runtime_error("Missing .end for function: " + current_function);
        }

        verify_functions();
        verify_labels();
    }

    Program get_program() {
        return program;
    }

    void write_bytecode(const std::string& filename) {
        CIR cir;
        cir.load_program(program);
        std::vector<uint8_t> bytecode = cir.to_bytecode();

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open output file: " + filename);
        }

        file.write(reinterpret_cast<char*>(bytecode.data()),
                   static_cast<std::streamsize>(bytecode.size()));
        file.close();
    }
};
