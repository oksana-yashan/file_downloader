#include <curl/curl.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <include/Chunk.h>
#include <include/Downloader.h>

class CurlDownloader : public Downloader
{
  public:
    CurlDownloader(const std::string& url) : url(url) {}
    ~CurlDownloader() override;

    void setUp() override;
    size_t getFileSize() override;
    bool downloadFile(const std::string& outputFile, int parallelTasks, std::uint64_t fileSize) override;

    std::string url;
};
