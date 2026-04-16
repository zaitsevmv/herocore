#include "raft.h"
#include "async/async_task.h"
#include "async/reactor.h"
#include "network/network_utils.h"
#include "tcp/tcp_requests.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace NUtils;

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
    void AppendEntries();
private:
    NAsync::TAsyncTask<bool> SendSocket(const std::string& data, int target);

    void Heartbeat();

    NAsync::TReactorPtr Reactor_;

    std::vector<int> KnownHostsSockets_;
    std::atomic_uint32_t CurrentTerm_ = 0;
    std::atomic<EHostStatus> CurrentStatus_ = EHostStatus::Follower;
    std::atomic_uint32_t CandidateVotes_ = 0;
};

TRaft::TImpl::TImpl(NAsync::TReactorPtr reactor) 
    :Reactor_(reactor) {}

TRaft::TImpl::~TImpl() = default;

void TRaft::TImpl::RequestVote() {
    CurrentTerm_++;
    CandidateVotes_ = 1;
    CurrentStatus_ = EHostStatus::Candidate;
    for (auto host: KnownHostsSockets_) {
        auto requestCoro = SendSocket("i start new term", host);
        requestCoro.Run();
    }
    // wait timer
    if (CurrentStatus_ == EHostStatus::Candidate) {
        CurrentStatus_ = EHostStatus::Leader;
    }
}

void TRaft::TImpl::AppendEntries() {
    Heartbeat();
}

NAsync::TAsyncTask<bool> TRaft::TImpl::SendSocket(const std::string& data, int target) {
    TRaftMessage msg{
        .SenderStatus = static_cast<std::underlying_type<EHostStatus>::type>(CurrentStatus_.load()),
        .SenderTerm = CurrentTerm_,
        .Message = data
    };
    auto msgString = msg.Serialize();
    if (!co_await SendDataAsync(Reactor_, target, msgString)) {
        // some retry logic, or new connection and retry
    }
    co_return true;
}

TRaft::TRaft(NAsync::TReactorPtr reactor)
    : Pimpl(std::make_unique<TRaft::TImpl>(reactor)) {}

void TRaft::AppendEntries() {
    return Pimpl->AppendEntries();
}

void TRaft::RequestVote() {
    return Pimpl->RequestVote();
}
