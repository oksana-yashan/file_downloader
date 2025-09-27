#include <include/BoostUtils.h>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <iostream>
#include <regex>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;
using asio::awaitable;
using asio::use_awaitable;

static constexpr auto PORT = "80";
static constexpr auto URL_REGEX = R"(^(?:https?://)?(?:www\.)?([^/]+)(/.*)?$)";

std::uint64_t getFileSizeByBoost(const std::string& host, const std::string& target, const std::string& port,
                                 int maxRetries)
{
    auto tryRequest = [&host, &target, &port, maxRetries](http::verb method) -> std::optional<std::uint64_t> {
        try
        {
            asio::io_context ioContext;
            tcp::resolver resolver(ioContext);
            auto endpoint = resolver.resolve(host, port);

            tcp::socket socket(ioContext);
            asio::connect(socket, endpoint);

            http::request<http::empty_body> request{method, target, 11};
            request.set(http::field::host, host);
            request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            request.set(http::field::accept, "*/*");
            request.set(http::field::connection, "close");

            http::write(socket, request);

            beast::flat_buffer buffer;
            http::response_parser<http::empty_body> parser;
            parser.skip(true); // skip http response body
            http::read_header(socket, buffer, parser);

            auto response = parser.get();
            if (response.result() != http::status::ok)
                return std::nullopt;

            if (auto it = response.find(http::field::content_length); it != response.end())
            {
                return std::stoull(std::string(it->value()));
            }

            return std::nullopt;
        }
        catch (std::exception& e)
        {
            std::cout << "File size request exception: " << e.what() << std::endl;
            return std::nullopt;
        }
    };

    for (int attempt = 1; attempt <= maxRetries; ++attempt)
    {
        // Firstly, try HEAD request
        if (auto size = tryRequest(http::verb::head))
            return *size;

        // If HEAD fails, try GET
        if (auto size = tryRequest(http::verb::get))
            return *size;

        std::cout << "Attempt " << attempt << " failed, retrying...\n";
    }

    std::cout << "Error: unable to determine file size after " << maxRetries << " attempts\n";
    return 0;
}

awaitable<bool> downloadChunk(const std::string& host, const std::string& port, const std::string& target,
                              Chunk& chunk)
{
    try
    {
        tcp::resolver resolver(co_await asio::this_coro::executor);
        auto endpoint = co_await resolver.async_resolve(host, port, use_awaitable);

        tcp::socket socket(co_await asio::this_coro::executor);
        co_await asio::async_connect(socket, endpoint, use_awaitable);

        // Build request with Range
        http::request<http::empty_body> request{http::verb::get, target, 11};
        request.set(http::field::host, host);
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.set(http::field::range,
                    "bytes=" + std::to_string(chunk.start) + "-" + std::to_string(chunk.end));

        co_await http::async_write(socket, request, use_awaitable);

        beast::flat_buffer buffer;
        http::response<http::vector_body<char>> response;
        co_await http::async_read(socket, buffer, response, use_awaitable);

        if (response.result() == http::status::partial_content || response.result() == http::status::ok)
        {
            chunk.data = std::move(response.body());
            co_return true;
        }

        std::cout << "Failed to download chunk " << chunk.start << "-" << chunk.end << " : "
                  << response.result_int() << std::endl;
        co_return false;

        beast::error_code errorCode;
        socket.shutdown(tcp::socket::shutdown_both, errorCode);
    }
    catch (std::exception& e)
    {
        std::cout << "Chunk download exception: " << e.what() << std::endl;
        co_return false;
    }
}

bool downloadFileByBoost(const std::string& host, const std::string& target, const std::string& port,
                         const std::string& outputFile, int parallelTasks, std::uint64_t fileSize,
                         int maxRetries)
{
    try
    {
        auto chunks = createChunks(parallelTasks, fileSize);

        asio::io_context ioContext;
        std::vector<std::future<bool>> results;

        for (auto& chunk : chunks)
        {
            // Launch each chunk as a coroutine with retry logic
            results.push_back(asio::co_spawn(
                ioContext,
                [&host, &port, &target, &chunk, maxRetries]() -> asio::awaitable<bool> {
                    for (int attempt = 1; attempt <= maxRetries; ++attempt)
                    {
                        try
                        {
                            bool ok = co_await downloadChunk(host, port, target, chunk);
                            if (ok && chunk.data.size())
                                co_return true;

                            std::cout << "Chunk [" << chunk.start << "-" << chunk.end << "] failed (attempt "
                                      << attempt << "), retrying...\n";
                        }
                        catch (const std::exception& e)
                        {
                            std::cout << "Chunk [" << chunk.start << "-" << chunk.end
                                      << "] exception: " << e.what() << "\n";
                        }
                    }
                    co_return false;
                },
                asio::use_future));
        }

        ioContext.run();

        for (auto& future : results)
        {
            if (!future.get())
            {
                std::cout << "Download failed: some chunks could not be retrieved\n";
                return false;
            }
        }

        return writeFile(outputFile, chunks, fileSize);
    }
    catch (const std::exception& e)
    {
        std::cout << "Download failed: " << e.what() << std::endl;
        return false;
    }
}

BoostDownloader::BoostDownloader(const std::string& url)
{
    parseUrl(url, host, target);
}

size_t BoostDownloader::getFileSize()
{
    return getFileSizeByBoost(host, target, PORT, 3);
}

bool BoostDownloader::downloadFile(const std::string& outputFile, int parallelTasks, uint64_t fileSize)
{
    std::cout << "Downloading file via Boost coroutines..." << std::endl;
    return downloadFileByBoost(host, target, PORT, outputFile, parallelTasks, fileSize, 3);
}

void BoostDownloader::parseUrl(const std::string& url, std::string& host, std::string& target)
{
    std::regex urlRegex(URL_REGEX, std::regex::icase);
    std::smatch match;
    if (std::regex_match(url, match, urlRegex))
    {
        host = match[1].str();
        target = match[2].matched ? match[2].str() : "/";
    }
    else
    {
        std::cerr << "Invalid URL format: " << url << "\n";
    }
}
