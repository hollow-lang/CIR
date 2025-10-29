#pragma once
#include <limits>

namespace Config {
    constexpr int REGISTER_COUNT = 256; // 256 Words = 2kb memory
    constexpr int STACK_SIZE = 1024*4; // 4kb

    constexpr int OpArgCount = 2;

    // Default Integer Type
    using DI_TYPE = uint32_t;
}
