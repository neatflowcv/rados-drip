#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <rados/librados.hpp>
#include <string>

namespace {

struct Options {
  std::string pool;
  std::optional<std::string> conf_path;
  std::optional<std::string> keyring_path;
  std::string client_name = "client.admin";
  std::string cluster_name = "ceph";
};

void PrintUsage(const char* program) {
  std::cerr << "Usage: " << program
            << " <pool> [-c ceph.conf] [--name client.admin]"
               " [--cluster ceph] [-k keyring]\n";
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

std::optional<Options> ParseOptions(int argc, char** argv) {
  Options options;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      return std::nullopt;
    }
    if (arg == "-c" || arg == "--conf") {
      if (++i >= argc) {
        std::cerr << arg << " requires a path\n";
        return std::nullopt;
      }
      options.conf_path = argv[i];
      continue;
    }
    if (arg == "-k" || arg == "--keyring") {
      if (++i >= argc) {
        std::cerr << arg << " requires a path\n";
        return std::nullopt;
      }
      options.keyring_path = argv[i];
      continue;
    }
    if (arg == "--name") {
      if (++i >= argc) {
        std::cerr << "--name requires a client entity name\n";
        return std::nullopt;
      }
      options.client_name = argv[i];
      continue;
    }
    if (arg == "--cluster") {
      if (++i >= argc) {
        std::cerr << "--cluster requires a cluster name\n";
        return std::nullopt;
      }
      options.cluster_name = argv[i];
      continue;
    }
    if (!arg.empty() && arg.front() == '-') {
      std::cerr << "unknown option: " << arg << '\n';
      return std::nullopt;
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

  try {
    std::uint64_t count = 0;
    for (auto object = ioctx.nobjects_begin(); object != ioctx.nobjects_end();
         ++object) {
      if (!object->get_nspace().empty()) {
        std::cout << object->get_nspace() << '\t';
      }
      std::cout << object->get_oid() << '\n';
      ++count;
    }
    std::cerr << "listed " << count << " object(s) from pool '" << options.pool
              << "'\n";
  } catch (const std::exception& error) {
    std::cerr << "listing objects failed: " << error.what() << '\n';
    cluster.shutdown();
    return 1;
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
