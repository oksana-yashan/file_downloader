#pragma once

#include <include/Chunk.h>

class Downloader
{
  public:
    virtual ~Downloader() {}

    virtual void setUp() = 0;
    virtual size_t getFileSize() = 0;
    virtual bool downloadFile(const std::string& outputFile, int parallelTasks, std::uint64_t fileSize) = 0;
};
