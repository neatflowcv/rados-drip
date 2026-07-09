#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "client/client.h"
#include "config/config.h"
#include "options/options.h"

namespace {

void PrintObjectList(const ListObjectsResult& results, std::ostream& output) {
  for (const auto& object : results.objects) {
    if (!object.Namespace().empty()) {
      output << object.Namespace() << '\t';
    }
    output << object.Name() << '\n';
  }
}

void PrintAllObjects(Client& client, const std::string& pool,
                     const std::optional<std::string>& initial_cursor,
                     std::chrono::milliseconds delay, std::ostream& output) {
  std::optional<std::string> cursor = initial_cursor;
  std::size_t object_count = 0;

  if (cursor) {
    std::cerr << "starting cursor: " << *cursor << '\n';
  }

  while (true) {
    const ListObjectsResult results = client.ListObjects(pool, cursor);
    PrintObjectList(results, output);
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
    if (delay > std::chrono::milliseconds{0}) {
      std::this_thread::sleep_for(delay);
    }
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
    if (options->output_path) {
      std::ofstream output(*options->output_path);
      if (!output) {
        throw std::runtime_error("failed to open output file: " +
                                 *options->output_path);
      }
      PrintAllObjects(client, options->pool, options->cursor, options->delay,
                      output);
    } else {
      PrintAllObjects(client, options->pool, options->cursor, options->delay,
                      std::cout);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
