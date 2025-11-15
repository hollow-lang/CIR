// Simple Dynamic Library Loading
#pragma once

#include <string>

#if defined(_WIN32)
#include <windows.h>
using LibHandle = HMODULE;
#else
#include <dlfcn.h>
using LibHandle = void *;
#endif

struct DynLib {
    LibHandle handle = nullptr;

    bool load(const std::string &path) {
#if defined(_WIN32)
        handle = LoadLibraryA(path.c_str());
#else
        handle = dlopen(path.c_str(), RTLD_LAZY);
#endif
        return handle != nullptr;
    }

    void unload() {
        if (!handle) return;
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        handle = nullptr;
    }

    template<typename T>
    T get(const std::string &symbol) {
#if defined(_WIN32)
        return reinterpret_cast<T>(GetProcAddress(handle, symbol.c_str()));
#else
        return reinterpret_cast<T>(dlsym(handle, symbol.c_str()));
#endif
    }
};