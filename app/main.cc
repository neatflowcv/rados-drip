#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <rados/librados.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "options/options.h"

namespace {

constexpr std::size_t kObjectListBatchSize = 1024;

std::string RadosError(int ret) {
  const int error_number = ret < 0 ? -ret : ret;
  return std::strerror(error_number);
}

std::runtime_error RadosRuntimeError(const std::string& message, int ret) {
  return std::runtime_error(message + ": " + RadosError(ret) + " (" +
                            std::to_string(ret) + ")");
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

void ConnectCluster(const Options& options, librados::Rados& cluster) {
  int ret = cluster.init2(options.client_name.c_str(),
                          options.cluster_name.c_str(), 0);
  if (ret < 0) {
    throw RadosRuntimeError("rados init failed", ret);
  }

  ret = cluster.conf_read_file(options.conf_path ? options.conf_path->c_str()
                                                 : nullptr);
  if (ret < 0) {
    throw RadosRuntimeError("reading Ceph config failed", ret);
  }

  const std::optional<std::string> keyring_path =
      options.keyring_path ? options.keyring_path : DefaultKeyringPath(options);
  if (keyring_path) {
    ret = cluster.conf_set("keyring", keyring_path->c_str());
    if (ret < 0) {
      throw RadosRuntimeError("setting keyring '" + *keyring_path + "' failed",
                              ret);
    }
  }

  ret = cluster.connect();
  if (ret < 0) {
    throw RadosRuntimeError("connecting to Ceph failed", ret);
  }
}

int ListObjects(const Options& options, librados::Rados& cluster) {
  int ret = 0;
  librados::IoCtx ioctx;
  ret = cluster.ioctx_create(options.pool.c_str(), ioctx);
  if (ret < 0) {
    std::cerr << "opening pool '" << options.pool
              << "' failed: " << RadosError(ret) << " (" << ret << ")\n";
    return 1;
  }
  ioctx.set_namespace(librados::all_nspaces);

  librados::ObjectCursor cursor = ioctx.object_list_begin();
  const librados::ObjectCursor end = ioctx.object_list_end();
  if (options.cursor) {
    if (!cursor.from_str(*options.cursor)) {
      std::cerr << "invalid cursor: " << *options.cursor << '\n';
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
  try {
    librados::Rados cluster;
    ConnectCluster(*options, cluster);
    const int ret = ListObjects(*options, cluster);
    cluster.shutdown();
    return ret;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
