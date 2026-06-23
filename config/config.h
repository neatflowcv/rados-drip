#pragma once

#include <string>

struct Config {
  std::string hosts;
  std::string key;
};

Config ReadConfig(const std::string& path);
