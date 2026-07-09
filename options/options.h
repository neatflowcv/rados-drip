#pragma once

#include <optional>
#include <string>

struct Options {
  std::string config_path;
  std::string pool;
  std::optional<std::string> cursor;
  std::optional<std::string> output_path;
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

void PrintUsage(const char* program);
std::optional<Options> ParseOptions(int argc, char** argv);
