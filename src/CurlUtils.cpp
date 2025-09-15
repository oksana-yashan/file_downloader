#include "include/CurlUtils.h"

static const auto CURL_CERT = "./../file_downloader/external/curl/cacert.pem";

size_t curlWriteCallback(void* ptr, size_t objSize, size_t n, void* userData)
{
    auto* chunk = static_cast<Chunk*>(userData);

    size_t totalSize = objSize * n;
    chunk->data.insert(chunk->data.end(), (char*)ptr, (char*)ptr + totalSize);

    return totalSize;
}

uint64_t getFileSize(const std::string& url)
{
    CURL* curl = curl_easy_init();
    double fileSize = 0;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, CURL_CERT);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &fileSize);
        }
        curl_easy_cleanup(curl);
    }
    return fileSize;
}

CURL* createEasyHandle(const std::string& url, Chunk& chunk)
{
    CURL* easy = curl_easy_init();
    curl_easy_setopt(easy, CURLOPT_CAINFO, CURL_CERT);
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &chunk);

    std::string range = std::to_string(chunk.start) + "-" + std::to_string(chunk.end);
    curl_easy_setopt(easy, CURLOPT_RANGE, range.c_str());

    return easy;
}

bool downloadFileByRange(const std::string& url, Chunk& chunk)
{
    CURL* curl = createEasyHandle(url, chunk);
    if (!curl)
        return false;

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

bool downloadFileByCreatingThreads(const std::string& url, const std::string& outputFile, int parallelTasks,
                                   uint64_t fileSize)
{
    auto chunks = createChunks(parallelTasks, fileSize);
    std::vector<std::thread> threads;

    for (auto& chunk : chunks)
    {
        threads.emplace_back([&url, &chunk]() {
            if (!downloadFileByRange(url, chunk))
            {
                std::cerr << "Chunk download failed: " << chunk.start << "-" << chunk.end << std::endl;
            }
        });
    }

    for (auto& t : threads)
        if (t.joinable())
            t.join();

    return writeFile(outputFile, std::move(chunks));
}

bool performMultiDownload(CURLM* multiHandle)
{
    int stillRunning = 0;
    curl_multi_perform(multiHandle, &stillRunning);

    while (stillRunning)
    {
        int numFds = 0;
        CURLMcode code = curl_multi_poll(multiHandle, nullptr, 0, 1000, &numFds);
        if (code != CURLM_OK)
        {
            std::cerr << "curl_multi_poll failed: " << curl_multi_strerror(code) << std::endl;
            return false;
        }
        curl_multi_perform(multiHandle, &stillRunning);
    }

    return true;
}

bool downloadFileByMultiCURL(const std::string& url, const std::string& outputFile, int parallelTasks,
                             uint64_t fileSize)
{
    auto chunks = createChunks(parallelTasks, fileSize);

    CURLM* multiHandle = curl_multi_init();
    std::vector<CURL*> easyHandles;

    for (auto& chunk : chunks)
    {
        CURL* easy = createEasyHandle(url, chunk);
        easyHandles.push_back(easy);
        curl_multi_add_handle(multiHandle, easy);
    }

    bool success = performMultiDownload(multiHandle);

    for (CURL* easy : easyHandles)
    {
        curl_multi_remove_handle(multiHandle, easy);
        curl_easy_cleanup(easy);
    }
    curl_multi_cleanup(multiHandle);

    return success && writeFile(outputFile, std::move(chunks));
}

bool writeFile(const std::string& outputFile, std::vector<Chunk>&& chunks)
{
    std::ofstream out(outputFile, std::ios::binary);
    if (!out)
    {
        std::cerr << "Failed to open output file: " << outputFile << std::endl;
        return false;
    }

    for (const auto& chunk : chunks)
    {
        out.write(chunk.data.data(), chunk.data.size());
    }

    return true;
}
