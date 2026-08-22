#include "http_message.h"

#include <functional>
#include <numeric>
#include <span>
#include <type_traits>
#include <vector>
#include <format>

namespace {

static constexpr std::string_view CRLF = "\n\r";

std::string_view SerializeMethod(const NHttp::EHttpMethod method) {
    switch (method) {
        case NHttp::EHttpMethod::GET:
            return "GET";
        case NHttp::EHttpMethod::POST:
            return "POST";
    }
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

} // namespace

namespace NHttp {

class TSerializeImpl {
public:
    TSerializeImpl();
    virtual ~TSerializeImpl() = default;

    void SetContent(const std::span<const char> content);
    void SetHeaders(const std::vector<THeader>& headers);
    virtual std::array<std::span<const char>, 2> Serialize() = 0;

protected:
    virtual void GenerateStartLine() = 0;
    virtual void GenerateHeaders() = 0;

protected:
    std::vector<THeader> const * Headers_ = nullptr;
    std::span<const char> Content_;
};


TSerializeImpl::TSerializeImpl() {}

void TSerializeImpl::SetContent(std::span<const char> content) {
    Content_ = content;
}

void TSerializeImpl::SetHeaders(const std::vector<THeader>& headers) {
    Headers_ = &headers;
}


class TRequestSerialize : public TSerializeImpl {
public:
    TRequestSerialize();
    ~TRequestSerialize();

    void SetMethod(const EHttpMethod method);
    void SetTarget(const std::string& target);
    std::array<std::span<const char>, 2> Serialize() override final;

private:
    void GenerateStartLine() override final;
    void GenerateHeaders() override final;

protected:
    EHttpMethod Method_ = EHttpMethod::GET;
    std::string Target_;
    std::string StartLine_;
    std::string HeadersString_;
    std::vector<char> Buffer_;
};

TRequestSerialize::TRequestSerialize() {};

TRequestSerialize::~TRequestSerialize() = default;

std::array<std::span<const char>, 2> TRequestSerialize::Serialize() {
    Buffer_.clear();
    Buffer_.reserve(StartLine_.size() + HeadersString_.size() + CRLF.size());

    Buffer_.assign(StartLine_.cbegin(), StartLine_.cend());
    Buffer_.insert(Buffer_.end(), HeadersString_.cbegin(), HeadersString_.cend());
    Buffer_.insert(Buffer_.end(), CRLF.cbegin(), CRLF.cend());

    return {Buffer_, Content_};
}

void TRequestSerialize::GenerateStartLine() {
    StartLine_ = std::format("{} {} HTTP/{}{}", SerializeMethod(Method_), Target_, "1.1", CRLF);
}

void TRequestSerialize::GenerateHeaders() {
    if (Headers_ == nullptr) {
        return;
    }
    HeadersString_ = std::accumulate(Headers_->cbegin(), Headers_->cend(), std::string(), [](std::string s_, const THeader& header) {
        return std::format("{}{}: {}{}", std::move(s_), header.Name, header.Value, CRLF);
    });
}


class TResponseSerialize : public TSerializeImpl {
public:
    TResponseSerialize();
    ~TResponseSerialize();

    void SetStatus(const EHttpStatus status);
    std::array<std::span<const char>, 2> Serialize() override final;

private:
    void GenerateStartLine() override final;
    void GenerateHeaders() override final;

protected:
    EHttpStatus Status_ = EHttpStatus::INTERNAL_SERVER_ERROR;
    std::string Target_;
    std::string StartLine_;
    std::string HeadersString_;
    std::vector<char> Buffer_;
};

TResponseSerialize::TResponseSerialize() {};

TResponseSerialize::~TResponseSerialize() = default;

std::array<std::span<const char>, 2> TResponseSerialize::Serialize() {
    Buffer_.clear();
    Buffer_.reserve(StartLine_.size() + HeadersString_.size() + CRLF.size());

    Buffer_.assign(StartLine_.cbegin(), StartLine_.cend());
    Buffer_.insert(Buffer_.end(), HeadersString_.cbegin(), HeadersString_.cend());
    Buffer_.insert(Buffer_.end(), CRLF.cbegin(), CRLF.cend());

    return {Buffer_, Content_};
}

void TResponseSerialize::GenerateStartLine() {
    StartLine_ = std::format("HTTP/{} {} {}{}", "1.1", std::to_string(static_cast<std::underlying_type_t<EHttpStatus>>(Status_)), SerializeStatusReason(Status_), CRLF);
}

void TResponseSerialize::GenerateHeaders() {
    if (Headers_ == nullptr) {
        return;
    }
    HeadersString_ = std::accumulate(Headers_->cbegin(), Headers_->cend(), std::string(), [](std::string s_, const THeader& header) {
        return std::format("{}{}: {}{}", std::move(s_), header.Name, header.Value, CRLF);
    });
}

} // namespace NHttp