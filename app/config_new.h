#pragma once

#include <string>

struct NewConfig {
  std::string hosts;
  std::string key;
};

NewConfig ReadNewConfig(const std::string& path);
