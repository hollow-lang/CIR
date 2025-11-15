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

struct FunctionAttributes {
    bool is_inline = false;
};

struct OpCodeInfo {
    OpType type;
    size_t arg_count;
};

class Assembler {
public:
    bool show_better_practice = true;
    std::unordered_map<std::string, OpCodeInfo> opcode_map;

private:
    std::unordered_map<std::string, std::unordered_map<std::string, size_t> > labels;
    std::unordered_map<std::string, FunctionAttributes> function_attributes;
    std::unordered_set<std::string> forward_label_refs;
    Program program;
    std::string current_function;
    size_t line_number = 0;

    CTEE ctee{};

    void init_opcode_map() {
        // 0 operands
        opcode_map["halt"] = {OpType::Halt, 0};
        opcode_map["nop"] = {OpType::Nop, 0};
        opcode_map["ret"] = {OpType::Ret, 0};

        // 1 operand
        opcode_map["not"] = {OpType::Not, 1};
        opcode_map["inc"] = {OpType::Inc, 1};
        opcode_map["dec"] = {OpType::Dec, 1};
        opcode_map["neg"] = {OpType::Neg, 1};
        opcode_map["push"] = {OpType::Push, 1};
        opcode_map["pushr"] = {OpType::PushReg, 1};
        opcode_map["pop"] = {OpType::Pop, 1};
        opcode_map["jmp"] = {OpType::Jmp, 1};
        opcode_map["call"] = {OpType::Call, 1};
        opcode_map["callx"] = {OpType::CallExtern, 1};
        opcode_map["local.get"] = {OpType::LocalGet, 1};

        // 2 operands
        opcode_map["mov"] = {OpType::Mov, 2};
        opcode_map["iadd"] = {OpType::IAdd, 2};
        opcode_map["isub"] = {OpType::ISub, 2};
        opcode_map["imul"] = {OpType::IMul, 2};
        opcode_map["idiv"] = {OpType::IDiv, 2};
        opcode_map["imod"] = {OpType::IMod, 2};
        opcode_map["and"] = {OpType::IAnd, 2};
        opcode_map["or"] = {OpType::IOr, 2};
        opcode_map["xor"] = {OpType::IXor, 2};
        opcode_map["shl"] = {OpType::Shl, 2};
        opcode_map["shr"] = {OpType::Shr, 2};
        opcode_map["icmp"] = {OpType::ICmp, 2};
        opcode_map["je"] = {OpType::Je, 2};
        opcode_map["jne"] = {OpType::Jne, 2};
        opcode_map["gt"] = {OpType::Gt, 2};
        opcode_map["gte"] = {OpType::Gte, 2};
        opcode_map["lt"] = {OpType::Lt, 2};
        opcode_map["lte"] = {OpType::Lte, 2};
        opcode_map["fadd"] = {OpType::FAdd, 2};
        opcode_map["fsub"] = {OpType::FSub, 2};
        opcode_map["fmul"] = {OpType::FMul, 2};
        opcode_map["fdiv"] = {OpType::FDiv, 2};
        opcode_map["fcmp"] = {OpType::FCmp, 2};
        opcode_map["cast"] = {OpType::Cast, 2};
        opcode_map["local.set"] = {OpType::LocalSet, 2};

        // 3 operands
        opcode_map["load"] = {OpType::Load, 3};
        opcode_map["store"] = {OpType::Store, 3};
    }

    std::string trim(const std::string &str) {
        size_t start = 0;
        while (start < str.size() && std::isspace(str[start])) start++;

        size_t end = str.size();
        while (end > start && std::isspace(str[end - 1])) end--;

        return str.substr(start, end - start);
    }

    std::vector<std::string> split(const std::string &str, char delim) {
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

    bool looks_like_number(const std::string &str) {
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

    Word parse_operand(const std::string &operand, bool is_jump = false) {
        (void) is_jump;
        std::string op = trim(operand);

        if (op.starts_with("comp(") && op.ends_with(")")) {
            std::string expr = op.substr(5, op.size() - 6);
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
            return Word::from_int(static_cast<int64_t>(labels[current_function][label]) - 1);
        }

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
                        case 'n': unescaped += '\n';
                            i++;
                            break;
                        case 't': unescaped += '\t';
                            i++;
                            break;
                        case 'r': unescaped += '\r';
                            i++;
                            break;
                        case '\\': unescaped += '\\';
                            i++;
                            break;
                        case '"': unescaped += '"';
                            i++;
                            break;
                        default: unescaped += str[i];
                            break;
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
                    case 'n': c = '\n';
                        break;
                    case 't': c = '\t';
                        break;
                    case 'r': c = '\r';
                        break;
                    case '0': c = '\0';
                        break;
                    case '\\': c = '\\';
                        break;
                    case '\'': c = '\'';
                        break;
                    default: c = op[2];
                        break;
                }
            }
            return Word::from_int(static_cast<int64_t>(c));
        }

        if (looks_like_number(op)) {
            throw std::runtime_error("Numeric literal '" + op + "' must be prefixed with '$' (e.g., $" + op + ")");
        }

        if (show_better_practice)
            std::cerr << "Note (line " << line_number << "): Operand '" << op <<
                    "' is being treated as a plain string.\n"
                    << "Mandatory prefixes:\n"
                    << "  - Numbers must start with $ (e.g., $123, $0xFF, $0b101)\n"
                    << "  - Labels must start with @ (e.g., @loop_start)\n"
                    << "  - Registers must be r0-r" << (Config::REGISTER_COUNT - 1) << "\n"
                    << "  - Strings:   \"text\"\n"
                    << "Optional for readability:\n"
                    << "  - IDs:       #name\n" << std::endl;

        return Word::from_string_owned(op);
    }

    void validate_instruction(const Op &op, const std::string &opcode, size_t expected_args) {
        size_t provided_args = 0;
        for (size_t i = 0; i < Config::OpArgCount; i++) {
            if (op.args[i].type != WordType::Null) {
                provided_args++;
            }
        }

        if (provided_args != expected_args) {
            throw std::runtime_error("Instruction '" + opcode + "' requires " +
                                     std::to_string(expected_args) + " operand(s), but " +
                                     std::to_string(provided_args) + " provided");
        }
    }

    void assemble_line(const std::string &line, Function &func) {
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

        const OpCodeInfo &opcode_info = opcode_map[opcode];

        Op op;
        op.type = opcode_info.type;

        for (size_t i = 0; i < Config::OpArgCount; i++) {
            op.args[i] = Word::from_null();
        }

        bool is_jump = (op.type == OpType::Jmp || op.type == OpType::Je || op.type == OpType::Jne);

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

        validate_instruction(op, opcode, opcode_info.arg_count);
        func.ops.push_back(op);
    }

    void verify_labels() {
        for (const auto &ref: forward_label_refs) {
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

        if (!program.functions.contains("main")) {
            throw std::runtime_error("No 'main' function defined");
        }
    }

    void inline_functions() {
        std::unordered_set<std::string> used_inline_functions;

        for (auto &[func_name, func]: program.functions) {
            std::vector<Op> new_ops;

            for (const auto &op: func.ops) {
                if (op.type == OpType::Call && op.args[0].has_flag(WordFlag::String)) {
                    std::string called_func = (const char *) op.args[0].as_ptr();

                    if (function_attributes.contains(called_func) &&
                        function_attributes[called_func].is_inline) {
                        if (!program.functions.contains(called_func)) {
                            throw std::runtime_error("Cannot inline undefined function: " + called_func);
                        }

                        const auto &inline_func = program.functions[called_func];

                        for (const auto &inline_op: inline_func.ops) {
                            if (inline_op.type == OpType::Ret) {
                                std::cerr << "[WARNING] Inlined function '" + called_func + "' returns from its body! It was removed automatically" << std::endl;
                                continue;
                            }
                            new_ops.push_back(inline_op);
                        }

                        used_inline_functions.insert(called_func);
                    } else {
                        new_ops.push_back(op);
                    }
                } else {
                    new_ops.push_back(op);
                }
            }

            func.ops = new_ops;
        }

        std::vector<std::string> functions_to_remove;
        for (const auto &[func_name, attrs]: function_attributes) {
            if (attrs.is_inline) {
                functions_to_remove.push_back(func_name);
            }
        }

        for (const auto &func_name: functions_to_remove) {
            program.functions.erase(func_name);
        }
    }

    FunctionAttributes parse_attributes(const std::string &attr_str) {
        FunctionAttributes attrs;
        std::vector<std::string> attr_list = split(attr_str, ' ');

        for (const auto &attr: attr_list) {
            std::string lower_attr = attr;
            std::transform(lower_attr.begin(), lower_attr.end(), lower_attr.begin(), ::tolower);

            if (lower_attr == "inline") {
                attrs.is_inline = true;
            } else {
                throw std::runtime_error("Unknown function attribute: " + attr);
            }
        }

        return attrs;
    }

public:
    Assembler() {
        init_opcode_map();
    }

    void assemble_file(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::string line;
        Function *current_func = nullptr;
        line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
            std::string cleaned = trim(line);

            if (cleaned.empty() || cleaned[0] == ';' || cleaned[0] == '#') continue;

            if (cleaned.find(".fn ") == 0) {
                size_t name_start = 4;
                std::string rest = trim(cleaned.substr(name_start));

                size_t first_space = rest.find(' ');
                std::string func_name;
                std::string attrs_str;

                if (first_space != std::string::npos) {
                    func_name = rest.substr(0, first_space);
                    attrs_str = trim(rest.substr(first_space + 1));
                } else {
                    func_name = rest;
                }

                current_function = func_name;

                if (current_function.empty()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": Function name cannot be empty");
                }

                if (program.functions.find(current_function) != program.functions.end()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                             ": Duplicate function definition: " + current_function);
                }

                if (!attrs_str.empty()) {
                    function_attributes[current_function] = parse_attributes(attrs_str);
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

            if (cleaned.find(".extern ") == 0) {
                program.required_externs.push_back(cleaned.substr(8));
                continue;
            }

            if (current_func != nullptr) {
                try {
                    assemble_line(cleaned, *current_func);
                } catch (const std::exception &e) {
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
        inline_functions();
    }

    void assemble_string(const std::string &source) {
        std::istringstream stream(source);
        std::string line;
        Function *current_func = nullptr;
        line_number = 0;

        while (std::getline(stream, line)) {
            line_number++;
            std::string cleaned = trim(line);

            if (cleaned.empty() || cleaned[0] == ';' || cleaned[0] == '#') continue;

            if (cleaned.find(".fn ") == 0) {
                size_t name_start = 4;
                std::string rest = trim(cleaned.substr(name_start));

                size_t first_space = rest.find(' ');
                std::string func_name;
                std::string attrs_str;

                if (first_space != std::string::npos) {
                    func_name = rest.substr(0, first_space);
                    attrs_str = trim(rest.substr(first_space + 1));
                } else {
                    func_name = rest;
                }

                current_function = func_name;

                if (current_function.empty()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": Function name cannot be empty");
                }

                if (program.functions.find(current_function) != program.functions.end()) {
                    throw std::runtime_error("Line " + std::to_string(line_number) +
                                             ": Duplicate function definition: " + current_function);
                }

                if (!attrs_str.empty()) {
                    function_attributes[current_function] = parse_attributes(attrs_str);
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
                } catch (const std::exception &e) {
                    throw std::runtime_error("Line " + std::to_string(line_number) + ": " + e.what());
                }
            }
        }

        if (current_func != nullptr) {
            throw std::runtime_error("Missing .end for function: " + current_function);
        }

        verify_functions();
        verify_labels();
        inline_functions();
    }

    Program get_program() {
        return program;
    }

    void write_bytecode(const std::string &filename) {
        CIR cir;
        cir.load_program(program);
        std::vector<uint8_t> bytecode = cir.to_bytecode();

        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open output file: " + filename);
        }

        file.write(reinterpret_cast<char *>(bytecode.data()),
                   static_cast<std::streamsize>(bytecode.size()));
        file.close();
    }
};
