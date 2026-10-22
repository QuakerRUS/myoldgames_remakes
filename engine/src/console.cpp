#include <vs/engine/console.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <iostream>
#endif

namespace vs {

void attach_parent_console() {
#ifdef _WIN32
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != nullptr && out != INVALID_HANDLE_VALUE && GetFileType(out) != FILE_TYPE_UNKNOWN) return;  // pipe or file
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return;
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    std::cout.clear();
    std::cerr.clear();
#endif
}

}  // namespace vs
