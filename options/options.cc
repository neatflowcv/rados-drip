#include "options.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

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

ParseResult ParseNamedOption(const std::string& arg, int argc, char** argv,
                             int& index, Options& options) {
  if (arg == "-c" || arg == "--conf") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a path");
    if (!value) {
      return ParseResult::kError;
    }
    options.conf_path = *value;
    return ParseResult::kParsed;
  }
  if (arg == "-k" || arg == "--keyring") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a path");
    if (!value) {
      return ParseResult::kError;
    }
    options.keyring_path = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--host") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a monitor host");
    if (!value) {
      return ParseResult::kError;
    }
    options.host = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--key") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a base64 CephX key");
    if (!value) {
      return ParseResult::kError;
    }
    options.key = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--name") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a client entity name");
    if (!value) {
      return ParseResult::kError;
    }
    options.client_name = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--cluster") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a cluster name");
    if (!value) {
      return ParseResult::kError;
    }
    options.cluster_name = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--cursor") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a cursor");
    if (!value) {
      return ParseResult::kError;
    }
    options.cursor = *value;
    return ParseResult::kParsed;
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
            << " <pool> [-c ceph.conf] [--name client.admin]"
               " [--cluster ceph] [-k keyring] [--host mon_host]"
               " [--key cephx_key] [--cursor cursor]\n";
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

    if (!options.pool.empty()) {
      std::cerr << "unexpected extra argument: " << arg << '\n';
      return std::nullopt;
    }
    options.pool = arg;
  }

  if (options.pool.empty()) {
    std::cerr << "pool is required\n";
    return std::nullopt;
  }
  if (options.host.has_value() != options.key.has_value()) {
    std::cerr << "--host and --key must be used together\n";
    return std::nullopt;
  }
  if (options.host && (options.conf_path || options.keyring_path)) {
    std::cerr << "--host/--key cannot be combined with --conf or --keyring\n";
    return std::nullopt;
  }
  return options;
}
