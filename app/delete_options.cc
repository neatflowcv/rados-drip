#include "delete_options.h"

#include <charconv>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <string_view>
#include <system_error>

#include "cli/parser.h"

namespace {

std::optional<std::chrono::milliseconds> ParseDelay(std::string_view value) {
  std::chrono::milliseconds::rep parsed = 0;
  const auto [ptr, error] = std::from_chars(value.begin(), value.end(), parsed);
  if (error != std::errc{} || ptr != value.end() || parsed < 0) {
    return std::nullopt;
  }
  return std::chrono::milliseconds{parsed};
}

bool ApplyFlag(const auto& entry, rados_delete::Options& options) {
  const auto& [flag, value] = entry;
  if (flag == "--name") {
    options.client_name = value;
  } else if (flag == "--cluster") {
    options.cluster_name = value;
  } else if (flag == "--delay-ms") {
    const auto delay = ParseDelay(value);
    if (!delay) {
      std::cerr << flag << " requires a non-negative millisecond count\n";
      return false;
    }
    options.delay = *delay;
  } else {
    std::cerr << "unknown option: " << flag << '\n';
    return false;
  }
  return true;
}

}  // namespace

namespace rados_delete {

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " <config> <pool> <objects> [--delay-ms milliseconds]"
               " [--name client.admin] [--cluster ceph]\n";
}

std::optional<Options> ParseOptions(int argc, char** argv) {
  const auto arguments = cli::Parse(argc, argv);
  if (!arguments) {
    std::cerr << arguments.error() << '\n';
    return std::nullopt;
  }
  if (arguments->positional.empty()) {
    std::cerr << "config is required\n";
    return std::nullopt;
  }
  if (arguments->positional.size() < 2) {
    std::cerr << "pool is required\n";
    return std::nullopt;
  }
  if (arguments->positional.size() < 3) {
    std::cerr << "objects is required\n";
    return std::nullopt;
  }
  constexpr std::size_t kRequiredArgumentCount = 3;
  if (arguments->positional.size() > kRequiredArgumentCount) {
    std::cerr << "unexpected extra argument: "
              << arguments->positional[kRequiredArgumentCount] << '\n';
    return std::nullopt;
  }

  Options options{.config_path = arguments->positional[0],
                  .pool = arguments->positional[1],
                  .objects_path = arguments->positional[2]};
  for (const auto& entry : arguments->flags) {
    if (!ApplyFlag(entry, options)) {
      return std::nullopt;
    }
  }
  return options;
}

}  // namespace rados_delete
