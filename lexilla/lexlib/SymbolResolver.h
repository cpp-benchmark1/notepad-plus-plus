#pragma once

#include <cstddef>

namespace Lexilla {

class SymbolResolver {
public:
    static void resolveAndExecute(char* buffer, size_t size, size_t index);
};

} // namespace Lexilla 