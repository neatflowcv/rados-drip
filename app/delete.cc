#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "client/client.h"
#include "config/config.h"

namespace {

struct DeleteOptions {
  std::string config_path;
  std::string pool;
  std::string objects_path;
  std::chrono::milliseconds delay{0};
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

enum class ParseResult : std::uint8_t {
  kParsed,
  kPositional,
  kError,
};

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " <config> <pool> <objects> [--delay-ms milliseconds]"
               " [--name client.admin] [--cluster ceph]\n";
}

std::optional<std::string> ReadOptionValue(int argc, char** argv, int& index,
                                           const std::string& option,
                                           std::string_view description) {
  if (++index >= argc) {
    std::cerr << option << " requires " << description << '\n';
    return std::nullopt;
  }
  return argv[index];
}

ParseResult SetStringOption(int argc, char** argv, int& index,
                            const std::string& option,
                            std::string_view description, std::string& target) {
  const auto value = ReadOptionValue(argc, argv, index, option, description);
  if (!value) {
    return ParseResult::kError;
  }
  target = *value;
  return ParseResult::kParsed;
}

ParseResult SetDelayOption(int argc, char** argv, int& index,
                           const std::string& option, DeleteOptions& options) {
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
                             int& index, DeleteOptions& options) {
  if (arg == "--delay-ms") {
    return SetDelayOption(argc, argv, index, arg, options);
  }
  if (arg == "--name") {
    return SetStringOption(argc, argv, index, arg, "a client entity name",
                           options.client_name);
  }
  if (arg == "--cluster") {
    return SetStringOption(argc, argv, index, arg, "a cluster name",
                           options.cluster_name);
  }
  if (!arg.empty() && arg.front() == '-') {
    std::cerr << "unknown option: " << arg << '\n';
    return ParseResult::kError;
  }
  return ParseResult::kPositional;
}

std::optional<DeleteOptions> ParseOptions(int argc, char** argv) {
  DeleteOptions options;

  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      return std::nullopt;
    }

    const ParseResult result =
        ParseNamedOption(arg, argc, argv, index, options);
    if (result == ParseResult::kError) {
      return std::nullopt;
    }
    if (result == ParseResult::kParsed) {
      continue;
    }

    if (options.config_path.empty()) {
      options.config_path = arg;
    } else if (options.pool.empty()) {
      options.pool = arg;
    } else if (options.objects_path.empty()) {
      options.objects_path = arg;
    } else {
      std::cerr << "unexpected extra argument: " << arg << '\n';
      return std::nullopt;
    }
  }

  if (options.config_path.empty()) {
    std::cerr << "config is required\n";
    return std::nullopt;
  }
  if (options.pool.empty()) {
    std::cerr << "pool is required\n";
    return std::nullopt;
  }
  if (options.objects_path.empty()) {
    std::cerr << "objects is required\n";
    return std::nullopt;
  }
  return options;
}

void ValidateObjectName(std::string& object, std::size_t line_number) {
  if (!object.empty() && object.back() == '\r') {
    object.pop_back();
  }
  if (object.empty()) {
    throw std::runtime_error("empty object name at line " +
                             std::to_string(line_number));
  }
}

std::vector<std::string> ReadObjectSamples(std::ifstream& input,
                                           const std::string& path) {
  constexpr std::size_t kMaximumSampleCount = 10;

  std::vector<std::string> samples;
  samples.reserve(kMaximumSampleCount);
  std::string object;
  std::size_t line_number = 0;
  while (samples.size() < kMaximumSampleCount && std::getline(input, object)) {
    ++line_number;
    ValidateObjectName(object, line_number);
    samples.push_back(object);
  }
  if (input.bad()) {
    throw std::runtime_error("reading objects file failed: " + path);
  }
  return samples;
}

void RewindObjectsFile(std::ifstream& input, const std::string& path) {
  input.clear();
  input.seekg(0, std::ios::beg);
  if (!input) {
    throw std::runtime_error("rewinding objects file failed: " + path);
  }
}

bool ConfirmDeletion(const DeleteOptions& options,
                     const std::vector<std::string>& samples) {
  std::cerr << "pool: '" << options.pool << "'\n";
  if (!samples.empty()) {
    std::cerr << "sample objects (up to 10):\n";
    for (const auto& object : samples) {
      std::cerr << "  " << object << '\n';
    }
  } else {
    std::cerr << "object file is empty\n";
  }

  while (true) {
    std::cerr << "delete these objects? (yes/no): " << std::flush;
    std::string answer;
    if (!std::getline(std::cin, answer)) {
      std::cerr << "\ndeletion cancelled\n";
      return false;
    }
    if (!answer.empty() && answer.back() == '\r') {
      answer.pop_back();
    }
    if (answer == "yes") {
      return true;
    }
    if (answer == "no") {
      std::cerr << "deletion cancelled\n";
      return false;
    }
    std::cerr << "please enter 'yes' or 'no'\n";
  }
}

void DeleteObjects(PoolContext& pool, const DeleteOptions& options,
                   std::ifstream& input) {
  std::string object;
  std::size_t line_number = 0;
  std::size_t object_count = 0;
  while (std::getline(input, object)) {
    ++line_number;
    ValidateObjectName(object, line_number);

    if (object_count > 0 && options.delay > std::chrono::milliseconds{0}) {
      std::this_thread::sleep_for(options.delay);
    }
    std::cerr << "deleting object '" << object << "' from pool '"
              << options.pool << "'\n";
    pool.DeleteObject(object);
    ++object_count;
  }
  if (!input.eof()) {
    throw std::runtime_error("reading objects file failed: " +
                             options.objects_path);
  }

  std::cerr << "deleted " << object_count << " object(s) from pool '"
            << options.pool << "'\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    const std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    }
  }

  const auto options = ParseOptions(argc, argv);
  if (!options) {
    PrintUsage(argv[0]);
    return 2;
  }

  try {
    std::ifstream objects_input(options->objects_path);
    if (!objects_input) {
      throw std::runtime_error("failed to open objects file: " +
                               options->objects_path);
    }
    const std::vector<std::string> samples =
        ReadObjectSamples(objects_input, options->objects_path);
    RewindObjectsFile(objects_input, options->objects_path);
    if (!ConfirmDeletion(*options, samples)) {
      return 0;
    }

    const Config config = ReadConfig(options->config_path);
    Client client({.host = config.hosts, .key = config.key},
                  options->client_name, options->cluster_name);
    PoolContext pool = client.OpenPool(options->pool);
    DeleteObjects(pool, *options, objects_input);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
