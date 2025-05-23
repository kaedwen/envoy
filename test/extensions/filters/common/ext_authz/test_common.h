#pragma once

#include "envoy/config/core/v3/base.pb.h"
#include "envoy/service/auth/v3/external_auth.pb.h"
#include "envoy/type/v3/http_status.pb.h"

#include "source/common/http/headers.h"
#include "source/extensions/filters/common/ext_authz/ext_authz_grpc_impl.h"

#include "test/extensions/filters/common/ext_authz/mocks.h"
#include "test/test_common/utility.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace ExtAuthz {

// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(const ResponsePtr& ptr, std::ostream* os);

// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(const Response& response, std::ostream* os);

struct KeyValueOption {
  std::string key;
  std::string value;
  bool append;
};

struct KeyValueAction {
  std::string key;
  std::string value;
  envoy::config::core::v3::HeaderValueOption::HeaderAppendAction append_action;
};

using KeyValueOptionVector = std::vector<KeyValueOption>;
using KeyValueActionVector = std::vector<KeyValueAction>;
using HeaderValueOptionVector = std::vector<envoy::config::core::v3::HeaderValueOption>;
using CheckResponsePtr = std::unique_ptr<envoy::service::auth::v3::CheckResponse>;

class TestCommon {
public:
  static Http::ResponseMessagePtr makeMessageResponse(const HeaderValueOptionVector& headers,
                                                      const std::string& body = std::string{});

  static CheckResponsePtr makeCheckResponse(Grpc::Status::GrpcStatus response_status,
                                            envoy::type::v3::StatusCode http_status_code,
                                            const std::string& body,
                                            const HeaderValueOptionVector& headers,
                                            const HeaderValueOptionVector& downstream_headers);

  static Response
  makeAuthzResponse(CheckStatus status, Http::Code status_code = Http::Code::OK,
                    const std::string& body = std::string{},
                    const HeaderValueOptionVector& headers = HeaderValueOptionVector{},
                    const HeaderValueOptionVector& downstream_headers = HeaderValueOptionVector{},
                    const absl::optional<Grpc::Status::GrpcStatus>& grpc_status = absl::nullopt);

  static HeaderValueOptionVector makeHeaderValueOption(KeyValueOptionVector&& headers);
  static HeaderValueOptionVector makeHeaderValueAction(KeyValueActionVector&& headers);

  static bool compareHeaderVector(const UnsafeHeaderVector& lhs, const UnsafeHeaderVector& rhs);
  static bool compareQueryParamsVector(const Http::Utility::QueryParamsVector& lhs,
                                       const Http::Utility::QueryParamsVector& rhs);
  static bool compareVectorOfHeaderName(const std::vector<std::string>& lhs,
                                        const std::vector<std::string>& rhs);
  static bool compareVectorOfUnorderedStrings(const std::vector<std::string>& lhs,
                                              const std::vector<std::string>& rhs);
};

MATCHER_P(AuthzErrorResponse, response, "") {
  // These fields should be always empty when the status is an error.
  if (!arg->headers_to_add.empty() || !arg->headers_to_append.empty() || !arg->body.empty()) {
    return false;
  }
  // HTTP status code should be always set to Forbidden.
  if (arg->status_code != Http::Code::Forbidden) {
    return false;
  }
  return arg->status == response.status;
}

MATCHER_P(AuthzResponseNoAttributes, response, "") {
  const bool equal_grpc_status = arg->grpc_status == response.grpc_status;
  const bool equal_status = arg->status == response.status;
  const bool equal_metadata =
      TestUtility::protoEqual(arg->dynamic_metadata, response.dynamic_metadata);
  if (!equal_metadata) {
    *result_listener << "\n"
                     << "==================Expected response dynamic metadata:==================\n"
                     << response.dynamic_metadata.DebugString()
                     << "------------------is not equal to actual dynamic metadata:-------------\n"
                     << arg->dynamic_metadata.DebugString()
                     << "=======================================================================\n";
  }
  return equal_grpc_status && equal_status && equal_metadata;
}

MATCHER_P(AuthzDeniedResponse, response, "") {
  if (arg->grpc_status != response.grpc_status) {
    return false;
  }
  if (arg->status != response.status) {
    return false;
  }
  if (arg->status_code != response.status_code) {
    return false;
  }
  if (arg->body.compare(response.body)) {
    return false;
  }
  // Compare headers_to_add.
  return TestCommon::compareHeaderVector(response.headers_to_add, arg->headers_to_add);
}

MATCHER_P(AuthzOkResponse, response, "") {
  if (arg->status != response.status) {
    return false;
  }

  if (arg->grpc_status != response.grpc_status) {
    return false;
  }

  if (!TestCommon::compareHeaderVector(response.headers_to_append, arg->headers_to_append)) {
    return false;
  }

  if (!TestCommon::compareHeaderVector(response.headers_to_add, arg->headers_to_add)) {
    return false;
  }

  if (!TestCommon::compareHeaderVector(response.response_headers_to_add,
                                       arg->response_headers_to_add)) {
    return false;
  }

  if (!TestCommon::compareHeaderVector(response.response_headers_to_set,
                                       arg->response_headers_to_set)) {
    return false;
  }

  if (!TestCommon::compareQueryParamsVector(response.query_parameters_to_set,
                                            arg->query_parameters_to_set)) {
    return false;
  }

  if (!TestCommon::compareVectorOfUnorderedStrings(response.query_parameters_to_remove,
                                                   arg->query_parameters_to_remove)) {
    return false;
  }

  if (!TestCommon::compareVectorOfHeaderName(response.headers_to_remove, arg->headers_to_remove)) {
    return false;
  }

  if (!TestUtility::protoEqual(arg->dynamic_metadata, response.dynamic_metadata)) {
    return false;
  }

  return true;
}

MATCHER_P(ContainsPairAsHeader, pair, "") {
  return arg->headers().get(pair.first)[0]->value().getStringView() == pair.second;
}

} // namespace ExtAuthz
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
