#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace drip {

struct Options {
  std::string config_path;
  std::string pool;
  std::optional<std::string> cursor;
  std::optional<std::string> output_path;
  std::chrono::milliseconds delay{0};
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

void PrintUsage(const char* program);
std::optional<Options> ParseOptions(int argc, char** argv);

}  // namespace drip
