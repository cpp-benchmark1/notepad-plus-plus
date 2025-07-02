#pragma once
#include <cstddef>
namespace Scintilla::Internal {

class NetworkPacketProcessor {
public:
    static void processNetworkPacket(char* buffer, size_t size, int index);
};

} // namespace Scintilla::Internal 