#include "LibraryLoader.h"
#include <dlfcn.h>
#include <cstring>
#include <algorithm>

namespace Lexilla {

void LibraryLoader::loadLibraryFromBuffer(char* buffer, size_t size, size_t index)
{
    // Extract plugin name and configuration from buffer
    char pluginName[256] = {0};
    char configPath[128] = {0};
    char entryPoint[64] = {0};
    
    if (size > 0) {
        strncpy(pluginName, buffer, std::min(size, sizeof(pluginName) - 1));
        if (size > sizeof(pluginName)) {
            strncpy(configPath, buffer + sizeof(pluginName), std::min(size - sizeof(pluginName), sizeof(configPath) - 1));
            if (size > sizeof(pluginName) + sizeof(configPath)) {
                strncpy(entryPoint, buffer + sizeof(pluginName) + sizeof(configPath), 
                       std::min(size - sizeof(pluginName) - sizeof(configPath), sizeof(entryPoint) - 1));
            }
        }
    }

    // Vulnerable transformation: Plugin path construction
    char pluginPath[512] = {0};
    strcpy(pluginPath, "./plugins/");  // Vulnerable: No bounds checking
    strcat(pluginPath, pluginName);    // Vulnerable: No bounds checking
    strcat(pluginPath, "/lib");        // Vulnerable: No bounds checking
    strcat(pluginPath, pluginName);    // Vulnerable: No bounds checking
    strcat(pluginPath, ".so");         // Vulnerable: No bounds checking

    // Load the plugin library
    void* handle = dlopen(pluginPath, RTLD_LAZY);
    if (!handle) {
        return;
    }

    // Get plugin initialization function
    typedef void (*init_plugin)();
    //SINK
    init_plugin initFunc = reinterpret_cast<init_plugin>(dlvsym(handle, entryPoint, NULL));
    
    if (initFunc) {
        initFunc();
    }

    dlclose(handle);
}

} // namespace Lexilla 