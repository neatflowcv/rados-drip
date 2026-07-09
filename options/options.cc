#include "options.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace {

enum class ParseResult : std::uint8_t {
  kParsed,
  kPositional,
  kError,
};

std::optional<std::string> ReadOptionValue(int argc, char** argv, int& index,
                                           const std::string& option,
                                           std::string_view value_description) {
  if (++index >= argc) {
    std::cerr << option << " requires " << value_description << '\n';
    return std::nullopt;
  }
  return argv[index];
}

ParseResult SetOptionValue(int argc, char** argv, int& index,
                           const std::string& option,
                           std::string_view value_description,
                           std::optional<std::string>& target) {
  const auto value =
      ReadOptionValue(argc, argv, index, option, value_description);
  if (!value) {
    return ParseResult::kError;
  }
  target = *value;
  return ParseResult::kParsed;
}

ParseResult SetOptionValue(int argc, char** argv, int& index,
                           const std::string& option,
                           std::string_view value_description,
                           std::string& target) {
  const auto value =
      ReadOptionValue(argc, argv, index, option, value_description);
  if (!value) {
    return ParseResult::kError;
  }
  target = *value;
  return ParseResult::kParsed;
}

ParseResult SetDelayOption(int argc, char** argv, int& index,
                           const std::string& option, Options& options) {
  const auto value = ReadOptionValue(argc, argv, index, option,
                                     "a non-negative millisecond count");
  if (!value) {
    return ParseResult::kError;
  }

  std::uint64_t parsed = 0;
  const char* const begin = value->data();
  const char* const end = begin + value->size();
  const auto [ptr, error] = std::from_chars(begin, end, parsed);
  using MillisecondRep = std::chrono::milliseconds::rep;
  if (error != std::errc{} || ptr != end ||
      parsed > static_cast<std::uint64_t>(
                   std::numeric_limits<MillisecondRep>::max())) {
    std::cerr << option << " requires a non-negative millisecond count\n";
    return ParseResult::kError;
  }

  options.delay =
      std::chrono::milliseconds{static_cast<MillisecondRep>(parsed)};
  return ParseResult::kParsed;
}

ParseResult ParseNamedOption(const std::string& arg, int argc, char** argv,
                             int& index, Options& options) {
  if (arg == "--name") {
    return SetOptionValue(argc, argv, index, arg, "a client entity name",
                          options.client_name);
  }
  if (arg == "--cluster") {
    return SetOptionValue(argc, argv, index, arg, "a cluster name",
                          options.cluster_name);
  }
  if (arg == "--cursor") {
    return SetOptionValue(argc, argv, index, arg, "a cursor", options.cursor);
  }
  if (arg == "--output") {
    return SetOptionValue(argc, argv, index, arg, "an output file",
                          options.output_path);
  }
  if (arg == "--delay-ms") {
    return SetDelayOption(argc, argv, index, arg, options);
  }
  if (!arg.empty() && arg.front() == '-') {
    std::cerr << "unknown option: " << arg << '\n';
    return ParseResult::kError;
  }
  return ParseResult::kPositional;
}

}  // namespace

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " <config> <pool> [--cursor cursor]"
               " [--output file]"
               " [--delay-ms milliseconds]"
               " [--name client.admin] [--cluster ceph]\n";
}

std::optional<Options> ParseOptions(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      return std::nullopt;
    }

    const ParseResult result = ParseNamedOption(arg, argc, argv, i, options);
    if (result == ParseResult::kError) {
      return std::nullopt;
    }
    if (result == ParseResult::kParsed) {
      continue;
    }

    if (options.config_path.empty()) {
      options.config_path = arg;
      continue;
    }
    if (options.pool.empty()) {
      options.pool = arg;
      continue;
    }
    std::cerr << "unexpected extra argument: " << arg << '\n';
    return std::nullopt;
  }

  if (options.config_path.empty()) {
    std::cerr << "config is required\n";
    return std::nullopt;
  }
  if (options.pool.empty()) {
    std::cerr << "pool is required\n";
    return std::nullopt;
  }
  return options;
}
