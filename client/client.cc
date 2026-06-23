#include "client/client.h"

#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

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

void InitCluster(librados::Rados& cluster, const std::string& client_name,
                 const std::string& cluster_name) {
  const int ret = cluster.init2(client_name.c_str(), cluster_name.c_str(), 0);
  if (ret < 0) {
    throw RadosRuntimeError("rados init failed", ret);
  }
}

void ConnectCluster(librados::Rados& cluster) {
  const int ret = cluster.connect();
  if (ret < 0) {
    throw RadosRuntimeError("connecting to Ceph failed", ret);
  }
}

}  // namespace

Client::Client(const InlineConnectionOptions& options,
               const std::string& client_name,
               const std::string& cluster_name) {
  InitCluster(cluster_, client_name, cluster_name);

  try {
    int ret = cluster_.conf_set("mon_host", options.host.c_str());
    if (ret < 0) {
      throw RadosRuntimeError("setting mon_host failed", ret);
    }

    ret = cluster_.conf_set("keyring", "");
    if (ret < 0) {
      throw RadosRuntimeError("disabling keyring search failed", ret);
    }

    ret = cluster_.conf_set("key", options.key.c_str());
    if (ret < 0) {
      throw RadosRuntimeError("setting CephX key failed", ret);
    }

    ConnectCluster(cluster_);
    connected_ = true;
  } catch (...) {
    cluster_.shutdown();
    throw;
  }
}

Client::~Client() {
  if (connected_) {
    cluster_.shutdown();
  }
}

ListObjectsResult Client::ListObjects(
    const std::string& pool, const std::optional<std::string>& cursor) {
  int ret = 0;
  librados::IoCtx ioctx;
  ret = cluster_.ioctx_create(pool.c_str(), ioctx);
  if (ret < 0) {
    throw RadosRuntimeError("opening pool '" + pool + "' failed", ret);
  }
  ioctx.set_namespace(librados::all_nspaces);

  librados::ObjectCursor current = ioctx.object_list_begin();
  const librados::ObjectCursor end = ioctx.object_list_end();
  if (cursor) {
    if (!current.from_str(*cursor)) {
      throw std::runtime_error("invalid cursor: " + *cursor);
    }
  }

  std::vector<librados::ObjectItem> objects;
  librados::ObjectCursor next;
  ret = ioctx.object_list(current, end, kObjectListBatchSize, {}, &objects,
                          &next);
  if (ret < 0) {
    throw RadosRuntimeError("listing objects failed", ret);
  }

  ListObjectsResult results;
  results.objects.reserve(objects.size());
  for (const auto& object : objects) {
    results.objects.emplace_back(object.oid, object.nspace);
  }
  results.next_cursor = next.to_str();
  results.is_end = ioctx.object_list_is_end(next);
  return results;
}
