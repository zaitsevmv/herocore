#include "files/files_operations.h"
#include "async/reactor.h"
#include "network/fd_utils.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>

using namespace std::chrono_literals;

static std::mt19937 GEN(1337);
std::uniform_int_distribution<> DIST(1e8, 1e9-1);

std::vector<std::string> CreateFiles(std::string path, size_t count) {
    const std::filesystem::path fsPath(path);
    auto filePath = fsPath / std::to_string(DIST(GEN));
    std::filesystem::create_directories(filePath.parent_path());

    std::vector<std::string> createdFiles(count);
    for (size_t i = 0ul; i < count; i++) {
        createdFiles[i] = filePath.string();
        std::ofstream ofs(filePath);
        filePath = fsPath / std::to_string(DIST(GEN));
    }
    return createdFiles;
}

void RemoveFiles(std::string path) {
    std::filesystem::remove_all(path);
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << "not enough arguments, use --help" << std::endl;
        return 0;
    }
    std::unordered_map<std::string, int> argsMap;
    argsMap["--ops"] = 10;
    argsMap["--string"] = 10000;
    argsMap["--files"] = 100; 
    std::string prevArg;
    for (int i = 1; i < argc; i++) {
        argsMap.emplace(argv[i], 0);
        if (!std::string_view(argv[i]).starts_with("--")) {
            try {
                int val = std::stoi(argv[i]);
                argsMap[prevArg] = val;
            } catch (...) {}
        }
        prevArg = argv[i];
    }

    if (argsMap.contains("--help")) {
        std::cout << "help:\n\n    --uring - to use async impl\n    --default - to use fstream" << std::endl;
        return 0;
    }

    const std::string testingPath = std::format("testing_{}", std::chrono::steady_clock::now().time_since_epoch().count());

    std::string longString;
    for (size_t i = 0; i < argsMap["--string"]; i++) {
        longString += std::to_string(DIST(GEN));
    }

    if (argsMap.contains("--default")) {
        auto totalTime = 0ns;
        for (size_t i = 0; i < argsMap["--ops"]; i++) {
            const auto workFiles = CreateFiles(testingPath, argsMap["--files"]);
            std::vector<std::future<void>> tasks;
            tasks.reserve(workFiles.size());
            const auto startTp = std::chrono::high_resolution_clock::now();

            for (const auto& file: workFiles) {
                tasks.push_back(std::async(std::launch::async,
                    [&longString, file](){
                        std::ofstream wf(file);
                        wf << longString;
                    }
                ));
            }

            for (auto& task: tasks) {
                task.wait();
            }

            const auto endTp = std::chrono::high_resolution_clock::now();
            RemoveFiles(testingPath);
            totalTime += (endTp - startTp);
        }
        std::cout << "Default test elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(totalTime/argsMap["--ops"]) << std::endl;
    }

    if (argsMap.contains("--uring")) {
        auto totalTime = 0ns;
        NAsync::TReactorPtr reactor = std::make_shared<NAsync::TReactor>();
        for (size_t i = 0; i < argsMap["--ops"]; i++) {
            const auto workFiles = CreateFiles(testingPath, argsMap["--files"]);
            std::stop_source ssource;
            std::jthread reactorThread(
                [reactor, stoken = ssource.get_token()]() {
                    reactor->Run(stoken);
                }
            );
            const auto startTp = std::chrono::high_resolution_clock::now();

            for (const auto& file: workFiles) {
                auto fd = NUtils::OpenFile(file);
                if (fd < 0) {
                    std::cerr << "error openning file for uring" << std::endl;
                    return 1;
                }
                NUtils::WriteFileAsync(reactor, fd, longString).Run();
            }

            ssource.request_stop();
            reactorThread.join();
            const auto endTp = std::chrono::high_resolution_clock::now();
            RemoveFiles(testingPath);
            totalTime += (endTp - startTp);
        }
        std::cout << "io_uring test elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(totalTime/argsMap["--ops"]) << std::endl;
    }
}