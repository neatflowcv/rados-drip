#pragma once

#include <stdexcept>
#include <string>

namespace client_detail {

std::runtime_error RadosRuntimeError(const std::string& message, int ret);

}  // namespace client_detail
