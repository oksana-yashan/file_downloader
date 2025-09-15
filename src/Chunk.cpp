#include <include/Chunk.h>

#include <iostream>

std::vector<Chunk> createChunks(int parallelTasks, uint64_t fileSize)
{
    std::vector<Chunk> chunks(parallelTasks);
    uint64_t chunkSize = fileSize / parallelTasks;

    for (int i = 0; i < parallelTasks; ++i)
    {
        chunks[i].start = i * chunkSize;
        chunks[i].end = (i == parallelTasks - 1) ? (fileSize - 1) : (chunks[i].start + chunkSize - 1);
        chunks[i].data.reserve(chunks[i].end - chunks[i].start + 1);
    }

    return chunks;
}

bool writeFile(const std::string& outputFile, const std::vector<Chunk>& chunks)
{
    std::ofstream out(outputFile, std::ios::binary);
    if (!out)
    {
        std::cout << "Failed to open output file: " << outputFile << std::endl;
        return false;
    }

    for (const auto& chunk : chunks)
    {
        out.write(chunk.data.data(), chunk.data.size());
    }
    return true;
}
