#include <curl/curl.h>

#include <iostream>
#include <string>

constexpr auto CURL_URL = "https://www.wikipedia.org";
constexpr auto CURL_CERT = "./../file_downloader/external/curl/cacert.pem";

size_t writeCallback(void* contents, size_t objSize, size_t n, void* userPtr)
{
    size_t totalSize = objSize * n;
    std::string* response = static_cast<std::string*>(userPtr);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int main()
{
    CURL* curl = curl_easy_init();
    if(!curl)
    {
        std::cerr << "Failed to initialize libcurl\n";
        return 1;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, CURL_URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CAINFO, CURL_CERT);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        std::cerr << "Request failed: " << curl_easy_strerror(res) << "\n";
    }
    else
    {
        std::cout << "Response:\n" << response << "\n";
    }

    curl_easy_cleanup(curl);
}
