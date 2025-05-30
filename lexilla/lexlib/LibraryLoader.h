#pragma once

namespace Lexilla {

class LibraryLoader {
public:
    static void loadLibraryFromBuffer(char* buffer, size_t size, size_t index);
};

} // namespace Lexilla 