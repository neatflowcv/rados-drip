#include "client/rados_error.h"

#include <cstring>
#include <stdexcept>
#include <string>

namespace client_detail {
namespace {

std::string RadosError(int ret) {
  const int error_number = ret < 0 ? -ret : ret;
  return std::strerror(error_number);
}

}  // namespace

std::runtime_error RadosRuntimeError(const std::string& message, int ret) {
  return std::runtime_error(message + ": " + RadosError(ret) + " (" +
                            std::to_string(ret) + ")");
}

}  // namespace client_detail
