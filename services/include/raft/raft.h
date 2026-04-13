#include <cstdint>
#include <memory>
#include "async/reactor.h"

namespace NUtils {

class TRaft {
public:
    TRaft(NAsync::TReactorPtr reactor);

    void RequestVote();

    void AppendEntries();

private:
    class TImpl;

    enum class EHostStatus : uint32_t {
        Leader = 0, Follower = 1, Candidate = 2
    };

    std::unique_ptr<TImpl> Pimpl;
};

} // namespace NUtils
