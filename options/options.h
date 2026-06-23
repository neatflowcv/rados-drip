#pragma once

#include <optional>
#include <string>

struct Options {
  std::string pool;
  std::string config_new_path;
  std::optional<std::string> cursor;
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

void PrintUsage(const char* program);
std::optional<Options> ParseOptions(int argc, char** argv);
