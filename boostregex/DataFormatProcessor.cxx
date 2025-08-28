#include "DataFormatProcessor.h"

namespace Scintilla::Internal {

void DataFormatProcessor::processDataFormat(char* buffer, size_t size, int index)
{
    char dest_buffer[10];
    char buf[1] = {'A'};

    // First transformation: Process data format and calculate offset
    if (size > 2) {
        // Extract format version and data type
        unsigned char formatVersion = buffer[0];
        unsigned char dataType = buffer[1];
        
        // Calculate offset based on format and data type
        if (formatVersion == 1) {  // Legacy format
            if (dataType == 0x01) {  // Text data
                index = index + 8;  // Text data offset
            } else if (dataType == 0x02) {  // Binary data
                index = index + 4;  // Binary data offset
            }
        } else if (formatVersion == 2) {  // New format
            if (dataType == 0x01) {  // Text data
                index = index - 2;  // New format text offset
            } else if (dataType == 0x02) {  // Binary data
                index = index - 6;  // New format binary offset
            }
        }
        
        // Remove format header
        for (size_t i = 0; i < size - 2; i++) {
            buffer[i] = buffer[i + 2];
        }
        size -= 2;
    }

    //SINK
    dest_buffer[index] = buf[0];  // Direct out-of-bounds write with attacker-controlled index
}

} // namespace Scintilla::Internal 