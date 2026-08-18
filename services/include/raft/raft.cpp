#include "raft.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <include/async/async_task.h>
#include <include/async/reactor.h>
#include <include/network/tcp_requests.h>
#include <include/thread_safe/queue.h>

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
    uint32_t MessageId = 0;

    std::string Message;

    static consteval size_t GetHeaderSize() {
        return sizeof(SenderStatus) + sizeof(SenderTerm) + sizeof(MessageId);
    }

    static uint32_t GenerateMessageId() {
        static std::random_device rd;
        static auto gen = std::mt19937(rd());
        static auto dist = std::uniform_int_distribution<unsigned long>(1, std::numeric_limits<uint32_t>().max());
        return dist(gen);
    }

    std::string Serialize() {
        std::string data;
        data.reserve(GetHeaderSize() + Message.size());
        auto hStatus = htonl(SenderStatus);
        auto hTerm = htonl(SenderTerm);
        auto hId = htonl(MessageId);
        data.insert(data.end(), reinterpret_cast<char*>(&hStatus), reinterpret_cast<char*>(&hStatus) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<char*>(&hTerm), reinterpret_cast<char*>(&hTerm) + sizeof(uint32_t));
        data.insert(data.end(), reinterpret_cast<char*>(&hId), reinterpret_cast<char*>(&hId) + sizeof(uint32_t));
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
    std::memcpy(&res.SenderTerm, msg.Data.data() + sizeof(res.SenderStatus) + sizeof(res.SenderTerm), sizeof(res.MessageId));
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
    NAsync::TAsyncTask<bool> HandleMessage(const TRaftMessage& message, size_t hostId);

    void Heartbeat();

    TQueueSafe<std::string> MasterRequestsQueue_;

    NAsync::TReactorPtr Reactor_;

    ssize_t LeaderId_ = -1;
    std::vector<TRaftHost> KnownHosts_;
    std::atomic_uint32_t CurrentTerm_ = 0;
    std::atomic<EHostStatus> CurrentStatus_ = EHostStatus::Follower;
    std::unordered_set<uint32_t> CandidateVotes_;
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
    CandidateVotes_.insert(TRaftMessage::GenerateMessageId());
    CurrentStatus_ = EHostStatus::Candidate;
    for (auto& host: KnownHosts_) {
        auto requestCoro = SendSocket("i start new term", host.HostSocket);
        requestCoro.Run();
    }
    // TODO: use config
    std::this_thread::sleep_for(20s);
    if (CurrentStatus_ == EHostStatus::Candidate && CandidateVotes_.size() > (KnownHosts_.size()+1)/2) {
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
    auto discovery = [this]() -> NAsync::TAsyncTask<bool> {
        do {
            uint16_t somePort = 3333;
            co_await AcceptNewHosts(3333);
        } while (true);
        co_return false;
    };
    std::jthread listeningThread(listener);
    std::jthread heartbeatThread(heartbeat);
    std::jthread discoveryThread(discovery);
}

void TRaft::TImpl::Heartbeat() {
    if (CurrentStatus_ == EHostStatus::Leader) {
        do {
            auto request = MasterRequestsQueue_.Front();
            std::vector<std::future<bool>> sendResults;
            // TODO: add to config
            auto deadline = TTimePoint::clock::now() + 3s;
            for (const auto& host: KnownHosts_) {
                auto requestResult = std::make_shared<std::promise<bool>>();
                sendResults.push_back(requestResult->get_future());
                auto coro = SendSocket(request, host.HostSocket);
                // probably better to do like this
                // auto sendResults.push_back(std::async([&coro]() -> NAsync::TAsyncTask<bool> {co_return co_await coro; }));
                coro.Subscribe([&requestResult](auto coroRes, auto e) mutable {
                    if (e) {
                        requestResult->set_exception(e);
                    }
                    requestResult->set_value(coroRes.get());
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
    uint32_t msgId = TRaftMessage::GenerateMessageId();
    TRaftMessage msg{
        .SenderStatus = static_cast<std::underlying_type<EHostStatus>::type>(CurrentStatus_.load()),
        .SenderTerm = CurrentTerm_,
        .MessageId = msgId,
        .Message = data,
    };
    auto msgString = msg.Serialize();
    ssize_t retryCount = 3;
    for (ssize_t i = 0; i < retryCount; i++) {
        if (co_await SendDataAsync(Reactor_, target, msgString)) { 
            break;
        } else if (i + 1 == retryCount) {
            co_return false;
        }
    }
    // i think that read will be much faster than timeout, so it wont be jammed
    uint32_t recvId = 0;
    do {
        auto resp = co_await ReadMessageAsync(Reactor_, target);
        TRaftMessage respMsg = Parse(resp);
        recvId = respMsg.MessageId;
    } while (recvId != msgId);
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
            KnownHosts_.push_back(std::move(newHost));
        } catch(const std::exception& e) {
            // handle bad client
        }
    }
    co_return true;
}

NAsync::TAsyncTask<bool> TRaft::TImpl::Listen(uint16_t port) {
    // TODO: make thread safe
    std::vector<std::future<std::shared_ptr<NUtils::TMessage>>> readResults;
    readResults.reserve(KnownHosts_.size());
    for (const auto& host: KnownHosts_) {
        auto requestResult = std::make_shared<std::promise<std::shared_ptr<NUtils::TMessage>>>();
        readResults.push_back(requestResult->get_future());
        auto msgCoro = ReadMessageAsync(Reactor_, host.HostSocket);
        msgCoro.Subscribe([requestResult](auto coroRes, auto e) mutable {
            if (e) {
                requestResult->set_exception(e);
            }
            requestResult->set_value(coroRes);
        });
        msgCoro.Run();
    }
    // TODO: add to config
    auto deadline = TTimePoint::clock::now() + 5s;
    for (auto i = 0ull; i < readResults.size(); i++) {
        auto& fut = readResults[i];
        auto status = fut.wait_until(deadline);
        if (status == std::future_status::ready && fut.valid()) {
            co_await SendSocket("ok", KnownHosts_[i].HostSocket);
            TRaftMessage msg = Parse(*fut.get().get());
            HandleMessage(msg, i);
        } else {
            co_await SendSocket("error", KnownHosts_[i].HostSocket); // TODO: specify error
            if (i == LeaderId_) {
                RequestVote();
            }
        }
    }
    co_return true;
}

NAsync::TAsyncTask<bool> TRaft::TImpl::HandleMessage(const TRaftMessage& message, size_t hostId) {
    if (CurrentTerm_ < message.SenderTerm) {
        CurrentStatus_ = TRaft::EHostStatus::Follower;
        CurrentTerm_ = message.SenderTerm;
    }
    switch (CurrentStatus_) {
        case EHostStatus::Leader: {
            break;
        };
        case EHostStatus::Candidate: {
            if (message.SenderStatus == static_cast<uint32_t>(EHostStatus::Leader)) {
                CurrentStatus_ = TRaft::EHostStatus::Follower;
                CandidateVotes_.clear();
                LeaderId_ = hostId;
            } else if (message.SenderStatus == static_cast<uint32_t>(EHostStatus::Follower)) {
                try {
                    uint32_t voteId = std::stoul(message.Message);
                    CandidateVotes_.insert(voteId);
                } catch(...) {}
            }
            break;
        };
        case EHostStatus::Follower: {
            if (message.SenderStatus == static_cast<uint32_t>(EHostStatus::Candidate)) {
                LeaderId_ = hostId;
                auto requestResult = std::make_shared<std::promise<bool>>();
                auto fut = requestResult->get_future();
                auto msgCoro = SendSocket(std::to_string(TRaftMessage::GenerateMessageId()), KnownHosts_[hostId].HostSocket);
                msgCoro.Subscribe([requestResult](auto coroRes, auto e) mutable {
                    if (e) {
                        requestResult->set_exception(e);
                    }
                    requestResult->set_value(coroRes.get());
                });
                msgCoro.Run();
                // TODO: think how it should work
                fut.wait_for(3s);
            }
            break;
        };
    }
    // handling
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
