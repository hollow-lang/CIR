#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <iostream>

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

    constexpr static void expect(Word &w, WordType etype, const std::string& msg = "No reason provided") {
        if (w.type != etype) {
            throw std::runtime_error("Expected " + std::to_string(static_cast<int>(etype)) + " but got " +
                                     std::to_string(static_cast<int>(w.type)) + ": " + msg);
        }
    }

    constexpr void expect(WordType etype) {
        Word::expect(*this, etype);
    }
};

static Word vregs[256] = {};

namespace cir_functions {
    void some_fn() {
        // IAdd
    }
}

int main() {

}

