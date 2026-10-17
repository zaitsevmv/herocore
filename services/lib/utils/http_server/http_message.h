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

class THttpMessage {
public:
    virtual void AddHeader(const std::string& name, const std::string& value) = 0;
    virtual std::string_view GetHeader(const std::string_view name) const = 0;

    virtual void SetContent(const std::span<const char> content) = 0;
    virtual void SetContentBuffer(const TContentBufferType& content) = 0;
    virtual void SetContentBuffer(TContentBufferType&& content) = 0;

    virtual void ParseStartLine(const std::string_view startLineStr) = 0;
    virtual void ParseHeaders(const std::string_view headersStr) = 0;
};
using THttpMessagePtr = std::shared_ptr<THttpMessage>;

class THttpRequest : public THttpMessage {
public:
    THttpRequest();
    ~THttpRequest();
    THttpRequest(THttpRequest&&) noexcept;
    THttpRequest& operator=(THttpRequest&&) noexcept;
    THttpRequest(const THttpRequest&) = delete;
    THttpRequest& operator=(const THttpRequest&) = delete;

    void AddHeader(const std::string& name, const std::string& value) override final;
    std::string_view GetHeader(const std::string_view name) const override final;

    void SetContent(const std::span<const char> content) override final;
    void SetContentBuffer(const TContentBufferType& content) override final;
    void SetContentBuffer(TContentBufferType&& content) override final;

    void SetMethod(const EHttpMethod method);
    EHttpMethod GetMethod() const;

    void SetTarget(const std::string& target);
    std::string GetTarget() const;

    void ParseStartLine(const std::string_view startLineStr) override final;
    void ParseHeaders(const std::string_view headersStr) override final;

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

class THttpResponse : public THttpMessage {
public:
    THttpResponse();
    ~THttpResponse();
    THttpResponse(THttpResponse&&) noexcept;
    THttpResponse& operator=(THttpResponse&&) noexcept;
    THttpResponse(const THttpResponse&) = delete;
    THttpResponse& operator=(const THttpResponse&) = delete;

    void AddHeader(const std::string& name, const std::string& value) override final;
    std::string_view GetHeader(const std::string_view name) const override final;

    void SetContent(const std::span<const char> content) override final;
    void SetContentBuffer(const TContentBufferType& buffer) override final;
    void SetContentBuffer(TContentBufferType&& buffer) override final;

    void SetStatus(const EHttpStatus status);
    EHttpStatus GetStatus() const;

    void ParseHeaders(const std::string_view headersStr) override final;
    void ParseStartLine(const std::string_view startLineStr) override final;

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
