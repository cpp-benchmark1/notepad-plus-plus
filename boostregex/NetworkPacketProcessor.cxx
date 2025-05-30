#include "NetworkPacketProcessor.h"

namespace Scintilla::Internal {

void NetworkPacketProcessor::processNetworkPacket(char* buffer, size_t size, int index)
{
    char dest_buffer[10];
    char buf[1] = {'B'};

    // First transformation: Process network packet header and calculate offset
    if (size > 4) {
        // Extract packet type and sequence number
        unsigned char packetType = buffer[0];
        unsigned char sequenceNum = buffer[1];
        
        // Calculate offset based on packet type and sequence
        if (packetType == 0x01) {  // Data packet
            index = index + (sequenceNum * 2);  // Offset based on sequence
        } else if (packetType == 0x02) {  // Control packet
            index = index - (sequenceNum / 2);  // Negative offset for control
        }
        
        // Remove header and shift data
        for (size_t i = 0; i < size - 4; i++) {
            buffer[i] = buffer[i + 4];
        }
        size -= 4;
    }

    // Second transformation: Apply network protocol encoding
    if (size > 0) {
        // Calculate protocol-specific offset
        unsigned char protocolVersion = buffer[0];
        index = index + (protocolVersion * 3);  // Protocol version affects index
        
        // Apply protocol encoding
        for (size_t i = 0; i < size; i++) {
            buffer[i] = buffer[i] ^ (0x55 + protocolVersion);  // Version-dependent XOR
        }
    }

    //SINK
    dest_buffer[index] = buf[0];  // Direct out-of-bounds write with attacker-controlled index
}

} // namespace Scintilla::Internal 