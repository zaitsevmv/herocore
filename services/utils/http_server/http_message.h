#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <span>
#include <variant>
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

enum class ParseState : uint16_t {
    Done,
    StartLine,
    Headers
};

class TSerializeImpl;

using TContentBufferType = std::variant<std::string, std::vector<char>>;

class THttpRequest {
public:
    THttpRequest();
    ~THttpRequest();
    THttpRequest(THttpRequest&&) noexcept;
    THttpRequest& operator=(THttpRequest&&) noexcept;
    THttpRequest(const THttpRequest&) = delete;
    THttpRequest& operator=(const THttpRequest&) = delete;

    void AddHeader(const std::string& name, const std::string& value);
    std::string_view GetHeader(const std::string_view name) const;

    void SetContent(const std::span<const char> content);
    void SetContentBuffer(const TContentBufferType& content);
    void SetContentBuffer(TContentBufferType&& content);

    void SetMethod(const EHttpMethod method);
    EHttpMethod GetMethod() const;

    void SetTarget(const std::string& target);
    std::string GetTarget() const;

    void ParseStartLine(const std::string_view startLineStr);
    void ParseHeaders(const std::string_view headersStr);

    std::array<std::span<const char>, 2> Serialize() const;

private:
    EHttpMethod Method_ = EHttpMethod::GET;
    std::string Target_;
    std::vector<THeader> Headers_;
    std::span<const char> Body_;
    TContentBufferType BodyBuffer_;
    std::unique_ptr<TSerializeImpl> SerializeImpl_;
};
using THttpRequestPtr = std::shared_ptr<THttpRequest>;

class THttpResponse {
public:
    THttpResponse();
    ~THttpResponse();
    THttpResponse(THttpResponse&&) noexcept;
    THttpResponse& operator=(THttpResponse&&) noexcept;
    THttpResponse(const THttpResponse&) = delete;
    THttpResponse& operator=(const THttpResponse&) = delete;

    void AddHeader(const std::string& name, const std::string& value);
    std::string_view GetHeader(const std::string_view name) const;

    void SetContent(const std::span<const char> content);
    void SetContentBuffer(const TContentBufferType& buffer);
    void SetContentBuffer(TContentBufferType&& buffer);

    void SetStatus(const EHttpStatus status);
    EHttpStatus GetStatus() const;

    void ParseHeaders(const std::string_view headersStr);
    void ParseStartLine(const std::string_view startLineStr);

    std::array<std::span<const char>, 2> Serialize() const;

private:
    EHttpStatus Status_ = EHttpStatus::INTERNAL_SERVER_ERROR;
    std::vector<THeader> Headers_;
    std::span<const char> Body_;
    TContentBufferType BodyBuffer_;
    std::unique_ptr<TSerializeImpl> SerializeImpl_;
};
using THttpResponsePtr = std::shared_ptr<THttpResponse>;

} // namespace NHttp
