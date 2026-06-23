#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <rados/librados.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::size_t kObjectListBatchSize = 1024;

struct Options {
  std::string pool;
  std::optional<std::string> conf_path;
  std::optional<std::string> keyring_path;
  std::optional<std::string> cursor;
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

enum class ParseResult : std::uint8_t {
  kParsed,
  kPositional,
  kError,
};

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " <pool> [-c ceph.conf] [--name client.admin]"
               " [--cluster ceph] [-k keyring] [--cursor cursor]\n";
}

std::string RadosError(int ret) {
  const int error_number = ret < 0 ? -ret : ret;
  return std::strerror(error_number);
}

std::optional<std::string> DefaultKeyringPath(const Options& options) {
  if (!options.conf_path) {
    return std::nullopt;
  }

  const std::filesystem::path conf_path(*options.conf_path);
  const std::filesystem::path conf_dir = conf_path.has_parent_path()
                                             ? conf_path.parent_path()
                                             : std::filesystem::path(".");
  const std::filesystem::path keyring_path =
      conf_dir / ("ceph." + options.client_name + ".keyring");
  if (std::filesystem::exists(keyring_path)) {
    return keyring_path.string();
  }
  return std::nullopt;
}

std::optional<std::string> ReadOptionValue(int argc, char** argv, int& index,
                                           const std::string& option,
                                           std::string_view value_description) {
  if (++index >= argc) {
    std::cerr << option << " requires " << value_description << '\n';
    return std::nullopt;
  }
  return argv[index];
}

ParseResult ParseNamedOption(const std::string& arg, int argc, char** argv,
                             int& index, Options& options) {
  if (arg == "-c" || arg == "--conf") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a path");
    if (!value) {
      return ParseResult::kError;
    }
    options.conf_path = *value;
    return ParseResult::kParsed;
  }
  if (arg == "-k" || arg == "--keyring") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a path");
    if (!value) {
      return ParseResult::kError;
    }
    options.keyring_path = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--name") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a client entity name");
    if (!value) {
      return ParseResult::kError;
    }
    options.client_name = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--cluster") {
    const auto value =
        ReadOptionValue(argc, argv, index, arg, "a cluster name");
    if (!value) {
      return ParseResult::kError;
    }
    options.cluster_name = *value;
    return ParseResult::kParsed;
  }
  if (arg == "--cursor") {
    const auto value = ReadOptionValue(argc, argv, index, arg, "a cursor");
    if (!value) {
      return ParseResult::kError;
    }
    options.cursor = *value;
    return ParseResult::kParsed;
  }
  if (!arg.empty() && arg.front() == '-') {
    std::cerr << "unknown option: " << arg << '\n';
    return ParseResult::kError;
  }
  return ParseResult::kPositional;
}

std::optional<Options> ParseOptions(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      return std::nullopt;
    }

    const ParseResult result = ParseNamedOption(arg, argc, argv, i, options);
    if (result == ParseResult::kError) {
      return std::nullopt;
    }
    if (result == ParseResult::kParsed) {
      continue;
    }

    if (!options.pool.empty()) {
      std::cerr << "unexpected extra argument: " << arg << '\n';
      return std::nullopt;
    }
    options.pool = arg;
  }

  if (options.pool.empty()) {
    std::cerr << "pool is required\n";
    return std::nullopt;
  }
  return options;
}

int ListObjects(const Options& options) {
  librados::Rados cluster;
  int ret = cluster.init2(options.client_name.c_str(),
                          options.cluster_name.c_str(), 0);
  if (ret < 0) {
    std::cerr << "rados init failed: " << RadosError(ret) << " (" << ret
              << ")\n";
    return 1;
  }

  ret = cluster.conf_read_file(options.conf_path ? options.conf_path->c_str()
                                                 : nullptr);
  if (ret < 0) {
    std::cerr << "reading Ceph config failed: " << RadosError(ret) << " ("
              << ret << ")\n";
    return 1;
  }

  const std::optional<std::string> keyring_path =
      options.keyring_path ? options.keyring_path : DefaultKeyringPath(options);
  if (keyring_path) {
    ret = cluster.conf_set("keyring", keyring_path->c_str());
    if (ret < 0) {
      std::cerr << "setting keyring '" << *keyring_path
                << "' failed: " << RadosError(ret) << " (" << ret << ")\n";
      return 1;
    }
  }

  ret = cluster.connect();
  if (ret < 0) {
    std::cerr << "connecting to Ceph failed: " << RadosError(ret) << " (" << ret
              << ")\n";
    return 1;
  }

  librados::IoCtx ioctx;
  ret = cluster.ioctx_create(options.pool.c_str(), ioctx);
  if (ret < 0) {
    std::cerr << "opening pool '" << options.pool
              << "' failed: " << RadosError(ret) << " (" << ret << ")\n";
    cluster.shutdown();
    return 1;
  }
  ioctx.set_namespace(librados::all_nspaces);

  librados::ObjectCursor cursor = ioctx.object_list_begin();
  const librados::ObjectCursor end = ioctx.object_list_end();
  if (options.cursor) {
    if (!cursor.from_str(*options.cursor)) {
      std::cerr << "invalid cursor: " << *options.cursor << '\n';
      cluster.shutdown();
      return 1;
    }
  }

  std::vector<librados::ObjectItem> objects;
  librados::ObjectCursor next;
  ret =
      ioctx.object_list(cursor, end, kObjectListBatchSize, {}, &objects, &next);
  if (ret < 0) {
    std::cerr << "listing objects failed: " << RadosError(ret) << " (" << ret
              << ")\n";
    cluster.shutdown();
    return 1;
  }

  for (const auto& object : objects) {
    if (!object.nspace.empty()) {
      std::cout << object.nspace << '\t';
    }
    std::cout << object.oid << '\n';
  }

  std::cerr << "listed " << objects.size() << " object(s) from pool '"
            << options.pool << "'\n";
  std::cerr << "next cursor: " << next.to_str() << '\n';
  if (ioctx.object_list_is_end(next)) {
    std::cerr << "end of object listing\n";
  }

  cluster.shutdown();
  return 0;
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
  return ListObjects(*options);
}
