#include "config/config.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace {

std::string_view Trim(std::string_view value) {
  while (!value.empty() && (value.front() == ' ' || value.front() == '\t' ||
                            value.front() == '\r')) {
    value.remove_prefix(1);
  }
  while (!value.empty() && (value.back() == ' ' || value.back() == '\t' ||
                            value.back() == '\r')) {
    value.remove_suffix(1);
  }
  return value;
}

void ValidateConfigMode(const std::string& path) {
  namespace fs = std::filesystem;

  std::error_code error;
  const fs::file_status status = fs::status(path, error);
  if (error) {
    throw std::runtime_error("reading config file status failed: " + path +
                             ": " + error.message());
  }
  if (!fs::is_regular_file(status)) {
    throw std::runtime_error("config file is not a regular file: " + path);
  }

  constexpr fs::perms kModeBits =
      fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
      fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec |
      fs::perms::others_read | fs::perms::others_write | fs::perms::others_exec;
  constexpr fs::perms kExpectedMode =
      fs::perms::owner_read | fs::perms::owner_write;

  if ((status.permissions() & kModeBits) != kExpectedMode) {
    throw std::runtime_error("config file must have mode 0600: " + path);
  }
}

}  // namespace

Config ReadConfig(const std::string& path) {
  ValidateConfigMode(path);

  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("opening config file failed: " + path);
  }

  Config config;
  std::string line;
  int line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;

    if (const std::string::size_type comment = line.find('#');
        comment != std::string::npos) {
      line.erase(comment);
    }

    const std::string_view trimmed_line = Trim(line);
    if (trimmed_line.empty()) {
      continue;
    }

    const std::string_view::size_type separator = trimmed_line.find('=');
    if (separator == std::string_view::npos) {
      throw std::runtime_error("invalid config line " +
                               std::to_string(line_number) + ": missing '='");
    }

    const std::string_view key = Trim(trimmed_line.substr(0, separator));
    const std::string_view value = Trim(trimmed_line.substr(separator + 1));
    if (key == "hosts") {
      config.hosts = std::string(value);
    } else if (key == "key") {
      config.key = std::string(value);
    }
  }

  if (config.hosts.empty()) {
    throw std::runtime_error("config file requires hosts: " + path);
  }
  if (config.key.empty()) {
    throw std::runtime_error("config file requires key: " + path);
  }
  return config;
}
