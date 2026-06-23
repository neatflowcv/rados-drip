#include <cstddef>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "client/client.h"
#include "config/config.h"
#include "options/options.h"

namespace {

void PrintObjectList(const ListObjectsResult& results) {
  for (const auto& object : results.objects) {
    if (!object.Namespace().empty()) {
      std::cout << object.Namespace() << '\t';
    }
    std::cout << object.Name() << '\n';
  }
}

void PrintAllObjects(Client& client, const std::string& pool,
                     const std::optional<std::string>& initial_cursor) {
  std::optional<std::string> cursor = initial_cursor;
  std::size_t object_count = 0;

  if (cursor) {
    std::cerr << "starting cursor: " << *cursor << '\n';
  }

  while (true) {
    const ListObjectsResult results = client.ListObjects(pool, cursor);
    PrintObjectList(results);
    object_count += results.objects.size();
    std::cerr << "next cursor: " << results.next_cursor << '\n';

    if (results.is_end) {
      break;
    }
    if (results.next_cursor.empty()) {
      throw std::runtime_error("object listing did not return a next cursor");
    }
    if (cursor && *cursor == results.next_cursor) {
      throw std::runtime_error("object listing cursor did not advance");
    }
    cursor = results.next_cursor;
  }

  std::cerr << "listed " << object_count << " object(s) from pool '" << pool
            << "'\n";
  std::cerr << "end of object listing\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    const std::string arg = argv[1];
    if (arg == "-h" || arg == "--help") {
      PrintUsage(argv[0]);
      return 0;
    }
  }

  const auto options = ParseOptions(argc, argv);
  if (!options) {
    PrintUsage(argv[0]);
    return 2;
  }
  try {
    const Config config = ReadConfig(options->config_path);
    Client client({.host = config.hosts, .key = config.key},
                  options->client_name, options->cluster_name);
    PrintAllObjects(client, options->pool, options->cursor);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
