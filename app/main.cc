#include <iostream>
#include <string>

#include "client/client.h"
#include "options/options.h"

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
    Client client(options->conf_path, options->keyring_path,
                  options->client_name, options->cluster_name);
    const ListObjectsResult results =
        client.ListObjects(options->pool, options->cursor);
    for (const auto& object : results.objects) {
      if (!object.Namespace().empty()) {
        std::cout << object.Namespace() << '\t';
      }
      std::cout << object.Name() << '\n';
    }

    std::cerr << "listed " << results.objects.size() << " object(s) from pool '"
              << options->pool << "'\n";
    std::cerr << "next cursor: " << results.next_cursor << '\n';
    if (results.is_end) {
      std::cerr << "end of object listing\n";
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
