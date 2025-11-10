#ifndef SCALC_H
#define SCALC_H
#include <stdexcept>
#include <string>
#include <unordered_map>

// Compile Time Expression Evaluator
class CTEE {
    std::string& s;
    size_t pos;

    std::unordered_map<std::string, double>& ctx; // label addresses and etc.

    [[nodiscard]] char peek() const { return pos < s.size() ? s[pos] : '\0'; }
    char get() { return pos < s.size() ? s[pos++] : '\0'; }

    void skip() { while (std::isspace(peek())) ++pos; }

    double number() {
        skip();
        size_t start = pos;
        while (std::isdigit(peek()) || peek() == '.') get();
        return std::stod(s.substr(start, pos - start));
    }

    std::string identifier() {
        skip();
        size_t start = pos;
        while (std::isalnum(peek()) || peek() == '_') get();
        return s.substr(start, pos - start);
    }

    double factor() {
        skip();
        if (std::isdigit(peek()) || peek() == '.') return number();
        if (std::isalpha(peek())) {
            std::string id = identifier();
            auto it = ctx.find(id);
            if (it == ctx.end()) throw std::runtime_error("Unknown variable: " + id);
            return it->second;
        }
        if (peek() == '(') {
            get();
            double val = expr();
            skip();
            if (get() != ')') throw std::runtime_error("Missing ')'");
            return val;
        }
        if (peek() == '-') { get(); return -factor(); }
        throw std::runtime_error("Unexpected character");
    }

    double term() {
        double val = factor();
        while (true) {
            skip();
            char op = peek();
            if (op == '*' || op == '/') {
                get();
                double rhs = factor();
                if (op == '*') val *= rhs;
                else val /= rhs;
            } else break;
        }
        return val;
    }

    double expr() {
        double val = term();
        while (true) {
            skip();
            char op = peek();
            if (op == '+' || op == '-') {
                get();
                double rhs = term();
                if (op == '+') val += rhs;
                else val -= rhs;
            } else break;
        }
        return val;
    }


public:
    inline static std::string dummy_str;
    inline static std::unordered_map<std::string, double> dummy_ctx;

    double eval(const std::string& str, std::unordered_map<std::string, double>& context) {
        s = str;
        pos = 0;
        ctx = context;
        return expr();
    }
    CTEE(std::string& str, std::unordered_map<std::string, double>& context) : s(str), pos(0), ctx(context) {}
    CTEE() : s(dummy_str), ctx(dummy_ctx), pos(0) {}
    ~CTEE() = default;

};


#endif //SCALC_H