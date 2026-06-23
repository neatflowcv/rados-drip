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
    return client.ListObjects(options->pool, options->cursor);
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
