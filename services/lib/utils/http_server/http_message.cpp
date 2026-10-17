#include "http_message.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <format>

namespace {

static constexpr std::string_view CRLF = "\r\n";

std::string_view SerializeMethod(const NHttp::EHttpMethod method) {
    switch (method) {
        case NHttp::EHttpMethod::GET:
            return "GET";
        case NHttp::EHttpMethod::POST:
            return "POST";
    }
}

NHttp::EHttpMethod ParseMethod(const std::string_view method) {
    if (method == "GET") {
        return NHttp::EHttpMethod::GET;
    } else if (method == "POST") {
        return NHttp::EHttpMethod::POST;
    }
    throw std::runtime_error("Error parsing HTTP method");
}

std::string_view SerializeStatusReason(const NHttp::EHttpStatus status) {
    switch (status) {
        case NHttp::EHttpStatus::OK:
            return "OK";
        case NHttp::EHttpStatus::BAD_REQUEST:
            return "Bad Request";
        case NHttp::EHttpStatus::INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
    }
}

NHttp::EHttpStatus ParseStatus(const std::string_view status) {
    try {
        const auto val = std::stoi(std::string(status));
        return static_cast<NHttp::EHttpStatus>(val);
    } catch (const std::exception& e) {
        throw std::runtime_error("Error parsing HTTP status");
    }
}

NHttp::THeader ParseHeader(const std::string_view line) {
    const auto separator = line.find(':');
    return NHttp::THeader{
        .Name = std::string(line.substr(0, separator)),
        .Value = std::string(line.substr(separator)),
    };
}

} // namespace

namespace NHttp {

class TSerializeImpl {
public:
    TSerializeImpl();
    virtual ~TSerializeImpl() = default;

    void SetContent(const std::span<const char> content);
    void SetHeaders(const std::vector<THeader>& headers);
    virtual void SetMethod(const EHttpMethod method) = 0;
    virtual void SetTarget(const std::string& target) = 0;
    virtual void SetStatus(const EHttpStatus status) = 0;
    virtual std::array<std::span<const char>, 2> Serialize() = 0;
protected:
    virtual void GenerateStartLine() = 0;
    void GenerateHeaders();
    void MakeBuffer();

protected:
    std::vector<THeader> const * Headers_ = nullptr;
    std::span<const char> Content_;
    std::string StartLine_;
    std::string HeadersString_;
    std::vector<char> Buffer_;
};


TSerializeImpl::TSerializeImpl() {}

void TSerializeImpl::SetContent(std::span<const char> content) {
    Content_ = content;
}

void TSerializeImpl::SetHeaders(const std::vector<THeader>& headers) {
    Headers_ = &headers;
}

void TSerializeImpl::MakeBuffer() {
    Buffer_.clear();
    Buffer_.reserve(StartLine_.size() + HeadersString_.size() + CRLF.size());

    Buffer_.assign(StartLine_.cbegin(), StartLine_.cend());
    Buffer_.insert(Buffer_.end(), HeadersString_.cbegin(), HeadersString_.cend());
    Buffer_.insert(Buffer_.end(), CRLF.cbegin(), CRLF.cend());
}

void TSerializeImpl::GenerateHeaders() {
    if (Headers_ == nullptr) {
        return;
    }
    HeadersString_ = std::accumulate(Headers_->cbegin(), Headers_->cend(), std::string(), [](std::string s_, const THeader& header) {
        return std::format("{}{}: {}{}", std::move(s_), header.Name, header.Value, CRLF);
    });
}


class TRequestSerialize : public TSerializeImpl {
public:
    TRequestSerialize();
    ~TRequestSerialize();

    void SetMethod(const EHttpMethod method) override final;
    void SetTarget(const std::string& target) override final;
    void SetStatus(const EHttpStatus status) override final;
    std::array<std::span<const char>, 2> Serialize() override final;

private:
    void GenerateStartLine() override final;

protected:
    EHttpMethod Method_ = EHttpMethod::GET;
    std::string Target_;
};

TRequestSerialize::TRequestSerialize() {};

TRequestSerialize::~TRequestSerialize() = default;

std::array<std::span<const char>, 2> TRequestSerialize::Serialize() {
    GenerateStartLine();
    GenerateHeaders();
    MakeBuffer();
    return {Buffer_, Content_};
}

void TRequestSerialize::GenerateStartLine() {
    StartLine_ = std::format("{} {} HTTP/{}{}", SerializeMethod(Method_), Target_, "1.1", CRLF);
}

void TRequestSerialize::SetMethod(const EHttpMethod method) {
    Method_ = std::move(method);
}

void TRequestSerialize::SetTarget(const std::string& target) {
    Target_ = target;
}

void TRequestSerialize::SetStatus(const EHttpStatus status) {}

class TResponseSerialize : public TSerializeImpl {
public:
    TResponseSerialize();
    ~TResponseSerialize();

    void SetMethod(const EHttpMethod method) override final;
    void SetTarget(const std::string& target) override final;
    void SetStatus(const EHttpStatus status) override final;
    std::array<std::span<const char>, 2> Serialize() override final;

private:
    void GenerateStartLine() override final;

protected:
    EHttpStatus Status_ = EHttpStatus::INTERNAL_SERVER_ERROR;
};

TResponseSerialize::TResponseSerialize() {};

TResponseSerialize::~TResponseSerialize() = default;

void TResponseSerialize::SetMethod(const EHttpMethod method) {}
void TResponseSerialize::SetTarget(const std::string& target) {}

void TResponseSerialize::SetStatus(const EHttpStatus status) {
    Status_ = std::move(status);
}

std::array<std::span<const char>, 2> TResponseSerialize::Serialize() {
    GenerateStartLine();
    GenerateHeaders();
    MakeBuffer();
    return {Buffer_, Content_};
}

void TResponseSerialize::GenerateStartLine() {
    StartLine_ = std::format("HTTP/{} {} {}{}", "1.1", std::to_string(static_cast<std::underlying_type_t<EHttpStatus>>(Status_)), SerializeStatusReason(Status_), CRLF);
}


THttpRequest::THttpRequest()
    : SerializeImpl_(std::make_unique<TRequestSerialize>()) {}
THttpRequest::~THttpRequest() = default;

THttpRequest::THttpRequest(THttpRequest&&) noexcept = default;
THttpRequest& THttpRequest::operator=(THttpRequest&&) noexcept = default;

void THttpRequest::AddHeader(const std::string& name, const std::string& value) {
    Headers_.emplace_back(name, value);
}

std::string_view THttpRequest::GetHeader(const std::string_view name) const {
    auto iter = std::ranges::find_if(Headers_, [&name](const THeader& header) {
        return header.Name == name;
    });
    if (iter != Headers_.end()) {
        return iter->Value;
    }
    return "";
}

void THttpRequest::SetContent(const std::span<const char> content) {
    SerializeImpl_->SetContent(content);
    Body_ = std::move(content);
}

void THttpRequest::SetContentBuffer(const TContentBufferType& buffer) {
    BodyBuffer_ = buffer;
}

void THttpRequest::SetContentBuffer(TContentBufferType&& buffer) {
    BodyBuffer_ = std::move(buffer);
}

void THttpRequest::SetMethod(const EHttpMethod method) {
    SerializeImpl_->SetMethod(method);
    Method_ = std::move(method);
}

EHttpMethod THttpRequest::GetMethod() const {
    return Method_;
}

void THttpRequest::SetTarget(const std::string& target) {
    SerializeImpl_->SetTarget(target);
    Target_ = target;
}

std::string THttpRequest::GetTarget() const {
    return Target_;
}

void THttpRequest::ParseStartLine(std::string_view startLineStr) {
    const auto method = startLineStr.find(' ');
    SetMethod(ParseMethod(startLineStr.substr(0, method)));

    startLineStr = startLineStr.substr(method + 1);
    const auto target = startLineStr.find(' ');
    SetTarget(std::string(startLineStr.substr(0, target)));

    // startLineStr = startLineStr.substr(target + 1);
    // const auto protocol = startLineStr.find(CRLF);
    // startLineStr = startLineStr.substr(protocol + 1);
}

void THttpRequest::ParseHeaders(std::string_view headersStr) {
    while (true) {
        const auto eol = headersStr.find(CRLF);
        const auto line = headersStr.substr(0, eol + CRLF.size());
        const auto sep = line.find(':');
        AddHeader(std::string(headersStr.substr(0, sep)), std::string(headersStr.substr(sep + 1)));
        headersStr = headersStr.substr(eol + CRLF.size());
        if (headersStr.starts_with(CRLF)) {
            return;
        }
    }
}

std::array<std::span<const char>, 2> THttpRequest::Serialize() const {
    SerializeImpl_->SetHeaders(Headers_);
    return SerializeImpl_->Serialize();
}

THttpResponse::THttpResponse()
    : SerializeImpl_(std::make_unique<TResponseSerialize>()) {}
THttpResponse::~THttpResponse() = default;
THttpResponse::THttpResponse(THttpResponse&&) noexcept = default;
THttpResponse& THttpResponse::operator=(THttpResponse&&) noexcept = default;

std::string_view THttpResponse::GetHeader(const std::string_view name) const {
    auto iter = std::ranges::find_if(Headers_, [&name](const THeader& header) {
        return header.Name == name;
    });
    if (iter != Headers_.end()) {
        return iter->Value;
    }
    return "";
}

void THttpResponse::AddHeader(const std::string& name, const std::string& value) {
    Headers_.emplace_back(name, value);
}

void THttpResponse::SetContent(const std::span<const char> content) {
    SerializeImpl_->SetContent(content);
    Body_ = std::move(content);
}

void THttpResponse::SetContentBuffer(const TContentBufferType& buffer) {
    BodyBuffer_ = buffer;
}

void THttpResponse::SetContentBuffer(TContentBufferType&& buffer) {
    BodyBuffer_ = std::move(buffer);
}

void THttpResponse::SetStatus(const EHttpStatus status) {
    SerializeImpl_->SetStatus(status);
    Status_ = std::move(status);
}

EHttpStatus THttpResponse::GetStatus() const {
    return Status_;
}

void THttpResponse::ParseStartLine(std::string_view startLineStr) {
    const auto protocol = startLineStr.find(' ');
    // SetMethod(ParseMethod(startLineStr.substr(0, protocol)));

    startLineStr = startLineStr.substr(protocol + 1);
    const auto status = startLineStr.find(' ');
    SetStatus(ParseStatus(startLineStr.substr(0, status)));

    // startLineStr = startLineStr.substr(status + 1);
    // const auto statusString = startLineStr.find(CRLF);
    // startLineStr = startLineStr.substr(statusString + 1);
}

void THttpResponse::ParseHeaders(std::string_view headersStr) {
    while (true) {
        const auto eol = headersStr.find(CRLF);
        const auto line = headersStr.substr(0, eol + CRLF.size());
        const auto sep = line.find(':');
        AddHeader(std::string(headersStr.substr(0, sep)), std::string(headersStr.substr(sep + 1)));
        headersStr = headersStr.substr(eol + CRLF.size());
        if (headersStr.starts_with(CRLF)) {
            return;
        }
    }
}
std::array<std::span<const char>, 2> THttpResponse::Serialize() const {
    SerializeImpl_->SetHeaders(Headers_);
    return SerializeImpl_->Serialize();
}

} // namespace NHttp
