#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace rados_delete {

struct Options {
  std::string config_path;
  std::string pool;
  std::string objects_path;
  std::chrono::milliseconds delay{0};
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

void PrintUsage(const char* program);
std::optional<Options> ParseOptions(int argc, char** argv);

}  // namespace rados_delete
