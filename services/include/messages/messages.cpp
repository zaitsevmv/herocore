#include "messages.h"

#include <netinet/in.h>
#include <cstring>
#include <stdexcept>

using namespace NUtils;

std::vector<char> TMessage::Serialize() const {
    std::vector<char> buffer;
    buffer.reserve(2 * sizeof(uint32_t) + TotalSize);
    
    auto hTotalSize = htonl(TotalSize);
    auto hChunkSize = htonl(ChunkSize);
    buffer.insert(buffer.end(), reinterpret_cast<char*>(&hTotalSize), reinterpret_cast<char*>(&hTotalSize) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<char*>(&hChunkSize), reinterpret_cast<char*>(&hChunkSize) + sizeof(uint32_t));
    buffer.insert(buffer.end(), Data.begin(), Data.end());
    return buffer;
}

std::string TMessage::ToString() const {
    return std::string(Data.data(), Data.size());
}

TMessage TMessage::Parse(std::span<const char> buffer) {
    if (buffer.size() < GetServiceDataSize()) {
        throw std::runtime_error("Incomplete header");
    }

    uint32_t hTotalSize;
    std::memcpy(&hTotalSize, buffer.data(), sizeof(uint32_t));
    auto totalSize = ntohl(hTotalSize);

    uint32_t hChunkSize;
    std::memcpy(&hChunkSize, buffer.data() + sizeof(uint32_t), sizeof(uint32_t));
    auto chunkSize = ntohl(hChunkSize);

    if (buffer.size() < GetServiceDataSize() + chunkSize) {
        throw std::runtime_error("Incomplete body");
    }

    TMessage msg;
    msg.TotalSize = totalSize;
    msg.ChunkSize = chunkSize;
    msg.Data.assign(buffer.data() + GetServiceDataSize(), buffer.data() + GetServiceDataSize() + chunkSize);
    return msg;
}
