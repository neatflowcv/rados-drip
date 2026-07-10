#include <chrono>
#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "app/delete_options.h"
#include "client/client.h"
#include "client/pool_context.h"
#include "config/config.h"

namespace {

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

bool ConfirmDeletion(const rados_delete::Options& options,
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

void DeleteObjects(PoolContext& pool, const rados_delete::Options& options,
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
      rados_delete::PrintUsage(argv[0]);
      return 0;
    }
  }

  const auto options = rados_delete::ParseOptions(argc, argv);
  if (!options) {
    rados_delete::PrintUsage(argv[0]);
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
