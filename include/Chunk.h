#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct Chunk
{
    uint64_t start;
    uint64_t end;
    std::vector<char> data;
};

std::vector<Chunk> createChunks(int parallelTasks, uint64_t fileSize);

bool writeFile(const std::string& outputFile, const std::vector<Chunk>& chunks);
