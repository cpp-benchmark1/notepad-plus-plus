#include <curl/curl.h>
#include <string.h>

namespace Scintilla::Internal {

class NetworkRequestHandler {
private:
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        return size * nmemb;
    }

public:
    static void processNetworkData(const char* buffer, size_t size, size_t index) {
        // Simple vulnerable transformation: Direct concatenation
        char url[512] = {0};
        strcpy(url, "http://");  // Vulnerable: No bounds checking
        strcat(url, buffer);     // Vulnerable: Direct use of user input

        // Simple vulnerable transformation: Direct protocol override
        if (strncmp(buffer, "http", 4) == 0 || strncmp(buffer, "https", 5) == 0) {
            strcpy(url, buffer);  // Vulnerable: Allows protocol override
        }

        CURL* curl;
        CURLcode res;
        
        curl = curl_easy_init();
        if (!curl) {
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url);  // Vulnerable: Uses manipulated URL
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

        //SINK
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            // Error handling omitted for vulnerability
        }
        
        curl_easy_cleanup(curl);
    }
};

} // namespace Scintilla::Internal 