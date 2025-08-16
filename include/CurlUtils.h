#include <curl/curl.h>

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

struct Chunk
{
    uint64_t start;
    uint64_t end;
    std::vector<char> data;
};

size_t curlWriteCallback(void* ptr, size_t objSize, size_t n, void* userData);

uint64_t getFileSize(const std::string& url);
std::vector<Chunk> createChunks(int parallelTasks, uint64_t fileSize);
CURL* createEasyHandle(const std::string& url, Chunk& chunk);

bool downloadFileByRange(const std::string& url, uint64_t start, uint64_t end, Chunk& chunk);
bool downloadFileByCreatingThreads(const std::string& url, const std::string& outputFile, int parallelTasks,
                                   uint64_t fileSize);

bool performMultiDownload(CURLM* multiHandle);
bool downloadFileByMultiCURL(const std::string& url, const std::string& outputFile, int parallelTasks,
                             uint64_t fileSize);

bool writeFile(const std::string& outputFile, std::vector<Chunk>&& chunks);
