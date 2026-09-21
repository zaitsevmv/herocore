#include "rpc_message.h"

namespace NRpc {

std::string TRpcMessage::Serialize() {

}


void TRpcStreamMessage::ParseChunk(const std::span<const char>& message) {

}

std::string TRpcStreamMessage::Serialize() {

}

EStreamState TRpcStreamMessage::GetState() noexcept {
    return StreamState_;
}

} // namespace NRpc

using namespace NRpc;

IRpcMessagePtr Parse(std::string message) {
    
}

IRpcMessagePtr Parse(const std::span<const char>& message) {

}
