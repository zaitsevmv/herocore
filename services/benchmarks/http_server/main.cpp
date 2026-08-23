#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include "utils/http_server/http_server.h"

#include <include/files/files_operations.h>
#include <include/async/timed_runner.h>
#include <include/network/fd_utils.h>
#include <include/thread_pool/thread_pool.h>

using namespace std::chrono_literals;

static std::mt19937 GEN(1337);
std::uniform_int_distribution<> DIST(1e8, 1e9-1);

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << "not enough arguments, use --help" << std::endl;
        return 0;
    }
    std::unordered_map<std::string, int> argsMap;
    argsMap["--ops"] = 10;
    argsMap["--string"] = 10000;
    argsMap["-pool"] = 8;
    std::string prevArg;
    for (int i = 1; i < argc; i++) {
        argsMap.emplace(argv[i], 0);
        if (!std::string_view(argv[i]).starts_with("--")) {
            try {
                int val = std::stoi(argv[i]);
                if (prevArg.ends_with("-pool")) {
                    argsMap["-pool"] = val;
                } else {
                    argsMap[prevArg] = val;
                }
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

    if (argsMap.contains("--uring-pool")) {
        auto totalTime = 0ns;
        auto tpool = std::make_shared<NAsync::TThreadPool>(argsMap["-pool"]);
        auto tpool_ = std::make_shared<NAsync::TThreadPool>(argsMap["-pool"]);
        NAsync::TReactorPtr reactor = std::make_shared<NAsync::TReactor>(tpool);
        for (size_t i = 0; i < argsMap["--ops"]; i++) {
            NHttp::THttpServer server(reactor);
            std::stop_source ssource;
            std::jthread reactorThread(
                [reactor, stoken = ssource.get_token()]() {
                    reactor->Run(stoken);
                }
            );
            const auto startTp = std::chrono::high_resolution_clock::now();

            server.Listen(tpool_, 3, 5555);

            // server.StopListen();
            // ssource.request_stop();
            // reactorThread.join();
            // tpool->Wait();
            // tpool_->Wait();
            // const auto endTp = std::chrono::high_resolution_clock::now();
            // totalTime += (endTp - startTp);
        }
        std::cout << "io_uring test elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(totalTime/argsMap["--ops"]) << std::endl;
    }
}