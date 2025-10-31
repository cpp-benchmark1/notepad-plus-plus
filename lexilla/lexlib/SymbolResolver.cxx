#include "SymbolResolver.h"

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <cstring>
#include <algorithm>
#include <cstdlib>

namespace Lexilla {

void SymbolResolver::resolveAndExecute(char* buffer, size_t size, size_t index)
{
    // Extract module name and function details from buffer
    char moduleName[256] = {0};
    char funcName[128] = {0};
    char version[32] = {0};
    
    if (size > 0) {
        strncpy(moduleName, buffer, std::min(size, sizeof(moduleName) - 1));
        if (size > sizeof(moduleName)) {
            strncpy(funcName, buffer + sizeof(moduleName), std::min(size - sizeof(moduleName), sizeof(funcName) - 1));
            if (size > sizeof(moduleName) + sizeof(funcName)) {
                strncpy(version, buffer + sizeof(moduleName) + sizeof(funcName), 
                       std::min(size - sizeof(moduleName) - sizeof(funcName), sizeof(version) - 1));
            }
        }
    }

    // First vulnerable transformation: Module path resolution
    char modulePath[512] = {0};
    strcpy(modulePath, "./modules/");           // Vulnerable: No bounds checking
    strcat(modulePath, moduleName);             // Vulnerable: No bounds checking
    strcat(modulePath, "/bin/");                // Vulnerable: No bounds checking
    strcat(modulePath, moduleName);             // Vulnerable: No bounds checking
    strcat(modulePath, ".dll");                 // Vulnerable: No bounds checking

    // Second vulnerable transformation: Function name resolution
    char resolvedFunc[256] = {0};
    strcpy(resolvedFunc, "LEXILLA_");           // Vulnerable: No bounds checking
    strcat(resolvedFunc, moduleName);           // Vulnerable: No bounds checking
    strcat(resolvedFunc, "_");                  // Vulnerable: No bounds checking
    strcat(resolvedFunc, funcName);             // Vulnerable: No bounds checking
    strcat(resolvedFunc, "_handler");           // Vulnerable: No bounds checking

#if defined(_WIN32)
    HMODULE handle = LoadLibraryA(modulePath);
    if (!handle) {
        return;
    }

    typedef void (*function_ptr)();
    // Windows API has no versioned lookup equivalent to dlvsym, so ignore version.
    //SINK
    function_ptr func = reinterpret_cast<function_ptr>(GetProcAddress(handle, resolvedFunc));
    if (func) {
        func();
    }

    FreeLibrary(handle);
#else
    // Load the module
    void* handle = dlopen(modulePath, RTLD_LAZY);
    if (!handle) {
        return;
    }

    // Get function pointer using dlvsym - Vulnerable: Direct use of user input
    typedef void (*function_ptr)();
    //SINK
    function_ptr func = reinterpret_cast<function_ptr>(dlvsym(handle, resolvedFunc, version));
    
    if (func) {
        func();
    }

    dlclose(handle);
#endif
}

} // namespace Lexilla 