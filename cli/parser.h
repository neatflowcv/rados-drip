#pragma once

#include <expected>
#include <string>
#include <utility>
#include <vector>

namespace cli {

using Flag = std::pair<std::string, std::string>;

struct Arguments {
  std::string program;
  std::vector<std::string> positional;
  std::vector<Flag> flags;
};

std::expected<Arguments, std::string> Parse(int argc, char** argv);

}  // namespace cli
