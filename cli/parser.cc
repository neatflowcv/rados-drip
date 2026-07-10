#include "parser.h"

#include <expected>
#include <string>
#include <utility>

namespace cli {

std::expected<Arguments, std::string> Parse(int argc, char** argv) {
  if (argc == 0) {
    return std::unexpected("program name is required");
  }

  Arguments arguments;
  arguments.program = argv[0];

  for (int index = 1; index < argc; ++index) {
    std::string argument = argv[index];
    if (argument.starts_with("--")) {
      const auto separator = argument.find('=');
      if (separator != std::string::npos) {
        arguments.flags.emplace_back(argument.substr(0, separator),
                                     argument.substr(separator + 1));
        continue;
      }
      ++index;
      if (index >= argc) {
        return std::unexpected(argument + " requires a value");
      }
      std::string value = argv[index];
      arguments.flags.emplace_back(std::move(argument), std::move(value));
      continue;
    }
    arguments.positional.push_back(std::move(argument));
  }

  return arguments;
}

}  // namespace cli
