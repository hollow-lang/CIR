#pragma once
#include <limits>

namespace Config {
    constexpr int REGISTER_COUNT = 256; // 256 Words = 2kb memory
    constexpr int STACK_SIZE = 1024 * 4; // 4kb
    constexpr int HEAP_SIZE = 1024 * 1024 * 64; // 64 kb

    constexpr int OpArgCount = 3;

    // Default Integer Type
    using DI_TYPE = uint32_t;

    inline auto VERSION = "1.0.0";
    inline auto AUTHORS = "zhrexx";
}
