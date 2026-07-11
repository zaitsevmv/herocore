#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <span>

namespace NUtils {

class TMessage {
public:
    uint32_t TotalSize = 0;
    uint32_t ChunkSize = 0;

    static size_t GetServiceDataSize() {
        return sizeof(TotalSize) + sizeof(ChunkSize);
    }

    std::vector<char> Data;

    std::vector<char> Serialize() const;

    std::string ToString() const;

    static TMessage Parse(std::span<const char> buffer);
};

} // namespace NUtils
