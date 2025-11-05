#pragma once
#include <limits>
#include <cstdint>

namespace Config {
    constexpr int REGISTER_COUNT = 256;
    constexpr int STACK_SIZE = 1024*4;
    constexpr int OpArgCount = 2;
    using DI_TYPE = uint32_t;
}
