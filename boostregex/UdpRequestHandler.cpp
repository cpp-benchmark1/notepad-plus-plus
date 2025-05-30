#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

namespace Scintilla::Internal {

class UdpRequestHandler {
public:
    static void sendUdpPacket(const char* buffer, size_t size, size_t index)
    {
        // First vulnerable transformation: Direct buffer copy
        char target[128] = {0};
        memcpy(target, buffer, size);  // Vulnerable: No bounds checking

        // Second vulnerable transformation: Port extraction
        char* portPtr = strrchr(target, '/');
        int port = 12349;  // Default port
        if (portPtr) {
            *portPtr = '\0';
            port = atoi(portPtr + 1);  // Vulnerable: No validation
        }

        int sockfd;
        struct sockaddr_in dest_addr;
        const char* msg = "connect";

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            return;
        }

        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(static_cast<uint16_t>(port));

        if (inet_pton(AF_INET, target, &dest_addr.sin_addr) <= 0) {
            close(sockfd);
            return;
        }

        //SINK
        sendto(sockfd, msg, strlen(msg), 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
        close(sockfd);
    }
};

} // namespace Scintilla::Internal 