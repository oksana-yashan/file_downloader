#include <include/Chunk.h>

#include <boost/asio/awaitable.hpp>

uint64_t getFileSizeByBoost(const std::string& host, const std::string& target,
                            const std::string& port = "80", int maxRetries = 3);

boost::asio::awaitable<bool> downloadChunk(const std::string& host, const std::string& port, const std::string& target,
                              Chunk& chunk);

bool downloadFileByBoost(const std::string& host, const std::string& target, const std::string& port,
                         const std::string& outputFile, int parallelTasks, uint64_t fileSize,
                         int maxRetries = 3);
