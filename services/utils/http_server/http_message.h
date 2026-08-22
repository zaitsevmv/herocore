#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <span>
#include <vector>

namespace NHttp {

struct THeader {
    std::string Name;
    std::string Value;
};

enum class EHttpMethod : uint16_t {
    GET,
    POST
};

enum class EHttpStatus : uint16_t {
    OK = 200,
    BAD_REQUEST = 400,
    INTERNAL_SERVER_ERROR = 500
};

class THttpMessage {
public:
    virtual void AddHeader(const std::string& name, const std::string& value) = 0;
    virtual void SetContent(const std::span<const char> content) = 0;
    virtual ~THttpMessage() = default;
protected:
    virtual std::span<const char> Serialize() = 0;
};

class TSerializeImpl;

class THttpRequest : public THttpMessage {
public:

    void AddHeader(const std::string& name, const std::string& value) override;
    void SetContent(const std::span<const char> content) override;
    void SetMethod(const EHttpMethod method);
private:
    std::span<const char> Serialize() override;

    std::vector<THeader> Headers_;
    std::span<const char> Body;
    std::unique_ptr<TSerializeImpl> RequestImpl_;
};

class THttpResponse : public THttpMessage {
public:
    void AddHeader(const std::string& name, const std::string& value) override;
    void SetContent(const std::span<const char> content) override;
private:
    std::span<const char> Serialize() override;

    std::vector<THeader> Headers_;
    std::span<const char> Body;
    std::unique_ptr<TSerializeImpl> RequestImpl_;
};

} // namespace NHttp
