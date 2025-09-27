#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

#include <include/BoostUtils.h>
#include <include/CurlUtils.h>

#include <boost/asio.hpp>
#include <boost/version.hpp>

static constexpr auto FILE_URL = "http://speedtest.tele2.net/10MB.zip";

bool parseCommandLine(QCoreApplication& app, std::string& outputFile, int& parallelTasks, std::string& url)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Parallel HTTP File Downloader");
    parser.addHelpOption();

    QCommandLineOption outputOption({"o", "output"}, "Output file", "downloadedFile.txt");
    parser.addOption(outputOption);
    QCommandLineOption parallelOption({"p", "parallel"}, "Number of parallel tasks", "4");
    parser.addOption(parallelOption);
    parser.addPositionalArgument("url", "The URL to download");
    parser.process(app);

    outputFile = parser.value(outputOption).toStdString();
    parallelTasks = parser.value(parallelOption).toInt();
    QStringList positionalArgs = parser.positionalArguments();
    url = positionalArgs.isEmpty() ? FILE_URL : positionalArgs.first().toStdString();
    std::cout << "File URL provided: " << FILE_URL << std::endl;

    if (outputFile.empty())
    {
        std::cerr << "Output file must be specified with -o option." << std::endl;
        parser.showHelp(1);
        return false;
    }

    if (parallelTasks <= 0)
    {
        std::cerr << "Number of parallel tasks must be greater than 0." << std::endl;
        parser.showHelp(1);
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    int parallelTasks;
    std::string url;
    std::string outputFile;
    if (!parseCommandLine(app, outputFile, parallelTasks, url))
    {
        return 1;
    }

    std::unique_ptr<Downloader> downloader = std::make_unique<BoostDownloader>(url);
    //std::unique_ptr<Downloader> downloader = std::make_unique<CurlDownloader>(url);

    const auto fileSize = downloader->getFileSize();
    if (fileSize <= 0)
    {
        std::cerr << "Could not determine file size!" << std::endl;
        return 1;
    }
    std::cout << "File size: " << fileSize << " bytes / " << (fileSize / 1024.0 / 1024.0) << " MB"
              << std::endl;

    bool success = downloader->downloadFile(outputFile, parallelTasks, fileSize);
    if (success)
    {
        std::cout << "File downloaded successfully!" << std::endl;
    }
    else
    {
        std::cerr << "File download failed!" << std::endl;
    }
}
