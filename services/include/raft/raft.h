#include <chrono>
#include <cstdint>
#include <memory>

#include <include/async/reactor.h>

namespace NUtils {

class TRaft {
public:
    using TTimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    enum class EHostStatus : uint32_t {
        Leader = 1, Follower = 2, Candidate = 3
    };

    TRaft(NAsync::TReactorPtr reactor);

    void RequestVote();

    void Start(uint16_t port);

    void AddRequest(const std::string& request);

private:
    class TImpl;

    std::unique_ptr<TImpl> Pimpl;
};

} // namespace NUtils
