#include <iostream>
#include <string>

#include "app/config_new.h"
#include "client/client.h"
#include "options/options.h"

namespace {

void PrintListObjectsResult(const ListObjectsResult& results,
                            const std::string& pool) {
  for (const auto& object : results.objects) {
    if (!object.Namespace().empty()) {
      std::cout << object.Namespace() << '\t';
    }
    std::cout << object.Name() << '\n';
  }

  std::cerr << "listed " << results.objects.size() << " object(s) from pool '"
            << pool << "'\n";
  std::cerr << "next cursor: " << results.next_cursor << '\n';
  if (results.is_end) {
    std::cerr << "end of object listing\n";
  }
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
    if (options->config_new_path) {
      const NewConfig config = ReadNewConfig(*options->config_new_path);
      Client client({.host = config.hosts, .key = config.key},
                    options->client_name, options->cluster_name);
      PrintListObjectsResult(client.ListObjects(options->pool, options->cursor),
                             options->pool);
    } else if (options->host && options->key) {
      Client client(
          {.host = options->host.value(), .key = options->key.value()},
          options->client_name, options->cluster_name);
      PrintListObjectsResult(client.ListObjects(options->pool, options->cursor),
                             options->pool);
    } else {
      Client client(options->conf_path, options->keyring_path,
                    options->client_name, options->cluster_name);
      PrintListObjectsResult(client.ListObjects(options->pool, options->cursor),
                             options->pool);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
