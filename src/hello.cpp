#include <hello.hpp>

#include <greeting.hpp>

namespace herocore {

std::string Hello::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SayHelloTo(request.GetArg("name"), UserType::kFirstTime);
}

}  // namespace herocore