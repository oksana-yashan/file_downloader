#include <include/Chunk.h>
#include <include/Downloader.h>

#include <boost/asio/awaitable.hpp>

#include <string>

class BoostDownloader : public Downloader
{
  public:
    BoostDownloader(const std::string& url);
    ~BoostDownloader() override {}

    void setUp() override {}
    size_t getFileSize() override;
    bool downloadFile(const std::string& outputFile, int parallelTasks, std::uint64_t fileSize) override;

  private:
    void parseUrl(const std::string& url, std::string& host, std::string& target);

    std::string host;
    std::string target;
};
