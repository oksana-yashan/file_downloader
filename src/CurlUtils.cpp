#include "include/CurlUtils.h"


static const auto CURL_CERT = "./../file_downloader/external/curl/cacert.pem";


size_t curlWriteCallback(void* ptr, size_t objSize, size_t n, void* userData)
{
    auto* chunk = static_cast<Chunk*>( userData );

    size_t totalSize = objSize * n;
    chunk->data.insert( chunk->data.end(), (char*)ptr, (char*)ptr + totalSize );

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

bool downloadFileByRange(const std::string& url, uint64_t start, uint64_t end, Chunk& chunk)
{
    CURL* curl = curl_easy_init();
    if ( !curl )
        return false;

    curl_easy_setopt(curl, CURLOPT_CAINFO, CURL_CERT);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

    std::string range = std::to_string(start) + "-" + std::to_string(end);
    curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return res == CURLE_OK;
}

bool downloadFile(const std::string& url, const std::string& outputFile, int parallelTasks, uint64_t fileSize)
{
    std::vector<Chunk> chunks(parallelTasks);
    std::vector<std::thread> threads;
    uint64_t chunkSize = fileSize / parallelTasks;

    for (int i = 0; i < parallelTasks; ++i)
    {
        long start = i * chunkSize;
        long end = (i == parallelTasks - 1) ? (fileSize - 1) : (start + chunkSize - 1);

        chunks[i].start = start;
        chunks[i].end = end;
        chunks[i].data.reserve(end - start + 1);

        threads.emplace_back(downloadFileByRange, url, start, end, std::ref(chunks[i]));
    }

    for (auto& t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    return writeFile(outputFile, std::move(chunks));
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
