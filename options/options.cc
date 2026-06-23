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

ParseResult ParseNamedOption(const std::string& arg, int argc, char** argv,
                             int& index, Options& options) {
  if (arg == "-c" || arg == "--conf") {
    return SetOptionValue(argc, argv, index, arg, "a path", options.conf_path);
  }
  if (arg == "-k" || arg == "--keyring") {
    return SetOptionValue(argc, argv, index, arg, "a path",
                          options.keyring_path);
  }
  if (arg == "--config-new") {
    return SetOptionValue(argc, argv, index, arg, "a path",
                          options.config_new_path);
  }
  if (arg == "--host") {
    return SetOptionValue(argc, argv, index, arg, "a monitor host",
                          options.host);
  }
  if (arg == "--key") {
    return SetOptionValue(argc, argv, index, arg, "a base64 CephX key",
                          options.key);
  }
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
               " [--cluster ceph] [-k keyring] [--config-new path]"
               " [--host mon_host] [--key cephx_key] [--cursor cursor]\n";
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
  if (options.config_new_path && (options.conf_path || options.keyring_path ||
                                  options.host || options.key)) {
    std::cerr << "--config-new cannot be combined with --conf, --keyring,"
                 " --host, or --key\n";
    return std::nullopt;
  }
  return options;
}
