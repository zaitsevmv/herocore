#include "raft.h"
#include "async/async_task.h"
#include "async/reactor.h"
#include "network/network_utils.h"
#include "tcp/tcp_requests.h"
#include "thread_safe/queue.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

using namespace NUtils;
using namespace std::literals;

struct TRaftHost {
    int HostSocket = 0;
    TRaft::EHostStatus HostStatus;
    TRaft::TTimePoint LastResponse;
};

struct TRaftMessage {
    uint32_t SenderStatus = 0;
    uint32_t SenderTerm = 0;

    std::string Message;

    static consteval size_t GetHeaderSize() {
        return sizeof(SenderStatus) + sizeof(SenderTerm);
    }

    std::string Serialize() {
        std::string data;
        data.reserve(GetHeaderSize() + Message.size());
        auto hStatus = htonl(SenderStatus);
        auto hTerm = htonl(SenderTerm);
        data.insert(data.end(), reinterpret_cast<char*>(&hStatus), reinterpret_cast<char*>(&hStatus) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<char*>(&hTerm), reinterpret_cast<char*>(&hTerm) + sizeof(uint32_t));
        std::copy(Message.begin(), Message.end(), std::back_inserter(data));
        return data;
    }
};

TRaftMessage Parse(const TMessage& msg) {
    TRaftMessage res;
    if (msg.Data.size() < TRaftMessage::GetHeaderSize()) {
        throw std::runtime_error("error: raft header missing");
    }
    std::memcpy(&res.SenderStatus, msg.Data.data(), sizeof(res.SenderStatus));
    std::memcpy(&res.SenderTerm, msg.Data.data() + sizeof(res.SenderStatus), sizeof(res.SenderTerm));
    res.Message = std::string(msg.Data.cbegin() + TRaftMessage::GetHeaderSize(), msg.Data.cend());
    return res;
}

class TRaft::TImpl {
public:
    TImpl(NAsync::TReactorPtr reactor);
    ~TImpl();

    void RequestVote();
    void Start(uint16_t port);
    void AddRequest(const std::string& request);
private:
    NAsync::TAsyncTask<bool> SendSocket(const std::string& data, int target);
    NAsync::TAsyncTask<bool> AcceptNewHosts(uint16_t port);
    NAsync::TAsyncTask<bool> Listen(uint16_t port);

    void Heartbeat();

    TQueueSafe<std::string> MasterRequestsQueue_;

    NAsync::TReactorPtr Reactor_;

    std::vector<TRaftHost> KnownHosts_;
    std::atomic_uint32_t CurrentTerm_ = 0;
    std::atomic<EHostStatus> CurrentStatus_ = EHostStatus::Follower;
    std::atomic_uint32_t CandidateVotes_ = 0;
};

TRaft::TImpl::TImpl(NAsync::TReactorPtr reactor) 
    :Reactor_(reactor) {}

TRaft::TImpl::~TImpl() = default;

void TRaft::TImpl::AddRequest(const std::string& request) {
    MasterRequestsQueue_.Push(request);
}

// how it should work: 1) vote started, 2) requests are handled so that followers add votes, 3) followers can send only to one candidate
void TRaft::TImpl::RequestVote() {
    CurrentTerm_++;
    CandidateVotes_ = 1;
    CurrentStatus_ = EHostStatus::Candidate;
    for (auto host: KnownHosts_) {
        auto requestCoro = SendSocket("i start new term", host.HostSocket);
        requestCoro.Run();
    }
    // TODO: use config
    std::this_thread::sleep_for(20s);
    if (CurrentStatus_ == EHostStatus::Candidate && CandidateVotes_ > (KnownHosts_.size()+1)/2) {
        CurrentStatus_ = EHostStatus::Leader;
    }
}

void TRaft::TImpl::Start(uint16_t port) {
    auto listener = [port, this]() -> NAsync::TAsyncTask<bool> {
        co_return co_await Listen(port);
    };
    auto heartbeat = [this]() {
        // TODO: add to config
        do {
            auto deadline = TTimePoint::clock::now() + 3s;
            Heartbeat();
            std::this_thread::sleep_until(deadline);
        } while (true);
    };
    std::jthread listeningThread(listener);
    std::jthread heartbeatThread(heartbeat);
}

void TRaft::TImpl::Heartbeat() {
    if (CurrentStatus_ == EHostStatus::Leader) {
        do {
            auto request = MasterRequestsQueue_.Front();
            std::vector<std::future<bool>> sendResults;
            // TODO: add to config
            auto deadline = TTimePoint::clock::now() + 3s;
            for (auto& host: KnownHosts_) {
                std::promise<bool> requestResult;
                sendResults.push_back(requestResult.get_future());
                auto coro = SendSocket(request, host.HostSocket);
                coro.Subscribe([&requestResult](auto coroRes, auto e) mutable {
                    if (e) {
                        requestResult.set_exception(e);
                    }
                    requestResult.set_value(coroRes.get());
                });
                coro.Run();
            }

            size_t successes = 0;
            for (auto& fut: sendResults) {
                auto status = fut.wait_until(deadline);
                // TODO: log
                successes += (status == std::future_status::ready && fut.valid() && fut.get());
            }
            MasterRequestsQueue_.Pop();
        } while (!MasterRequestsQueue_.Empty());
    }
}

NAsync::TAsyncTask<bool> TRaft::TImpl::SendSocket(const std::string& data, int target) {
    TRaftMessage msg{
        .SenderStatus = static_cast<std::underlying_type<EHostStatus>::type>(CurrentStatus_.load()),
        .SenderTerm = CurrentTerm_,
        .Message = data
    };
    auto msgString = msg.Serialize();
    if (!co_await SendDataAsync(Reactor_, target, msgString)) {
        // TODO: some retry logic, or new connection and retry
        co_return false;
    }
    co_return true;
}

NAsync::TAsyncTask<bool> TRaft::TImpl::AcceptNewHosts(uint16_t port) {
    auto thatSocket = CreateListeningSocket(port);
    {
        auto newConnection = co_await AcceptConnectionAsync(Reactor_, thatSocket);
        auto msg = co_await ReadMessageAsync(Reactor_, newConnection);
        try {
            TRaftMessage raftMsg = Parse(msg);
            TRaftHost newHost = {
                .HostSocket = newConnection,
                .HostStatus = static_cast<TRaft::EHostStatus>(raftMsg.SenderStatus),
                .LastResponse = TTimePoint::clock::now()
            };
            bool isReplaced = false;
            for (auto i = 0ull; i < KnownHosts_.size(); i++) {
                const auto& host = KnownHosts_[i];
                if (host.LastResponse > TTimePoint::clock::now() + 30s) {
                    KnownHosts_[i] = newHost;
                    isReplaced = true;
                    break;
                }
            }
            if (!isReplaced) {
                KnownHosts_.push_back(std::move(newHost));
            }
        } catch(const std::exception& e) {
            // handle bad client
        }
    }
    co_return true;
}

NAsync::TAsyncTask<bool> TRaft::TImpl::Listen(uint16_t port) {
    auto hostsSize = KnownHosts_.size();
    std::vector<std::future<std::shared_ptr<NUtils::TMessage>>> readResults;
    readResults.reserve(hostsSize);
    for (auto i = 0ull; i < hostsSize; i++) {
        const auto& host = KnownHosts_[i]; // TODO: thread safe
        std::promise<std::shared_ptr<NUtils::TMessage>> requestResult;
        readResults.push_back(requestResult.get_future());
        auto msgCoro = ReadMessageAsync(Reactor_, host.HostSocket);
        msgCoro.Subscribe([&requestResult](auto coroRes, auto e) mutable {
            if (e) {
                requestResult.set_exception(e);
            }
            requestResult.set_value(coroRes);
        });
        msgCoro.Run();
    }
    // TODO: add to config
    auto deadline = TTimePoint::clock::now() + 5s;
    for (auto i = 0ull; i < hostsSize; i++) {
        const auto& fut = readResults[i];
        auto status = fut.wait_until(deadline);
        if (status == std::future_status::ready && fut.valid()) {
            co_await SendSocket("ok", KnownHosts_[i].HostSocket);

            // handle request
        } else {
            co_await SendSocket("error", KnownHosts_[i].HostSocket); // TODO: specify error, handle possible timeouts
            if ("leader") { // TODO: add hosts ids
                RequestVote();
            }
        }
    }
    co_return true;
}

TRaft::TRaft(NAsync::TReactorPtr reactor)
    : Pimpl(std::make_unique<TRaft::TImpl>(reactor)) {}

void TRaft::Start(uint16_t port) {
    return Pimpl->Start(port);
}

void TRaft::RequestVote() {
    return Pimpl->RequestVote();
}

void TRaft::AddRequest(const std::string& request) {
    return Pimpl->AddRequest(request);
}
