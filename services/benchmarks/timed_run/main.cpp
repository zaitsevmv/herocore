#include "async/timed_runner.h"
#include "thread_pool/thread_pool.h"

#include <chrono>
#include <format>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

static std::mt19937 GEN(1337);
std::uniform_int_distribution<> DIST(10, 300);

std::vector<NAsync::TDurationType> CreateDurations(size_t count) {
    std::vector<NAsync::TDurationType> createdDurations(count);
    for (size_t i = 0ul; i < count; i++) {
        createdDurations[i] = std::chrono::milliseconds(DIST(GEN));
    }
    return createdDurations;
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << "not enough arguments, use --help" << std::endl;
        return 0;
    }
    std::unordered_map<std::string, int> argsMap;
    argsMap["--ops"] = 10;
    argsMap["--tasks"] = 1000;
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
        std::cout << "help:\n\n    --ops - restart counts   --tasks - created durations" << std::endl;
        return 0;
    }

    const std::string testingPath = std::format("testing_{}", std::chrono::steady_clock::now().time_since_epoch().count());

    std::string longString;
    for (size_t i = 0; i < argsMap["--string"]; i++) {
        longString += std::to_string(DIST(GEN));
    }

    if (argsMap.contains("--default")) {
        auto totalTime = 0ns;
        auto delta = 0ns;
        for (size_t i = 0; i < argsMap["--ops"]; i++) {
            const auto durations = CreateDurations(argsMap["--tasks"]);
            NAsync::TThreadPoolPtr tpool = std::make_shared<NAsync::TThreadPool>(1);
            NAsync::TTimedRunnerPtr runner = std::make_shared<NAsync::TTimedRunner>(tpool);

            const auto startTp = std::chrono::high_resolution_clock::now();

            for (const auto& dur: durations) {
                runner->AddToRunner([&delta, deadline = NAsync::TTimePointType::clock::now() + dur]() {
                    delta += (NAsync::TTimePointType::clock::now() - deadline);
                }, dur);
            }

            runner->RequestStop();
            tpool->Wait();
            const auto endTp = std::chrono::high_resolution_clock::now();

            totalTime += (endTp - startTp);
        }
        std::cout << "Default test elapsed " << std::chrono::duration_cast<std::chrono::milliseconds>(totalTime/argsMap["--ops"]) << std::endl;
        std::cout << "Default test task avg delta: " << std::chrono::duration_cast<std::chrono::nanoseconds>(delta/argsMap["--ops"]/argsMap["--tasks"]) << std::endl;
    }
}