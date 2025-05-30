#pragma once

namespace Scintilla::Internal {

class DataFormatProcessor {
public:
    static void processDataFormat(char* buffer, size_t size, int index);
};

} // namespace Scintilla::Internal 