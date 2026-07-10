#include "client/client.h"

#include <cstddef>
#include <cstring>
#include <optional>
#include <rados/librados.hpp>
#include <rados/rados_types.hpp>
#include <stdexcept>
#include <string>
#include <utility>
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

PoolContext Client::OpenPool(const std::string& pool,
                             const ObjectNamespace& object_namespace) {
  librados::IoCtx ioctx;
  const int ret = cluster_.ioctx_create(pool.c_str(), ioctx);
  if (ret < 0) {
    throw RadosRuntimeError("opening pool '" + pool + "' failed", ret);
  }
  ioctx.set_namespace(object_namespace.Value());
  return {pool, std::move(ioctx)};
}

ObjectNamespace::ObjectNamespace(std::string value)
    : value_(std::move(value)) {}

ObjectNamespace ObjectNamespace::All() {
  // rados_types.hpp defines this symbol, but include-cleaner cannot trace it.
  // NOLINTNEXTLINE(misc-include-cleaner)
  return ObjectNamespace{librados::all_nspaces};
}

const std::string& ObjectNamespace::Value() const { return value_; }

PoolContext::PoolContext(std::string pool, librados::IoCtx ioctx)
    : pool_(std::move(pool)), ioctx_(std::move(ioctx)) {}

const std::string& PoolContext::Name() const { return pool_; }

ListObjectsResult PoolContext::ListObjects(
    const std::optional<std::string>& cursor) {
  int ret = 0;

  librados::ObjectCursor current = ioctx_.object_list_begin();
  const librados::ObjectCursor end = ioctx_.object_list_end();
  if (cursor) {
    if (!current.from_str(*cursor)) {
      throw std::runtime_error("invalid cursor: " + *cursor);
    }
  }

  std::vector<librados::ObjectItem> objects;
  librados::ObjectCursor next;
  ret = ioctx_.object_list(current, end, kObjectListBatchSize, {}, &objects,
                           &next);
  if (ret < 0) {
    throw RadosRuntimeError("listing objects in pool '" + pool_ + "' failed",
                            ret);
  }

  ListObjectsResult results;
  results.objects.reserve(objects.size());
  for (const auto& object : objects) {
    results.objects.emplace_back(object.oid, object.nspace);
  }
  results.next_cursor = next.to_str();
  results.is_end = ioctx_.object_list_is_end(next);
  return results;
}

void PoolContext::DeleteObject(const std::string& object) {
  const int ret = ioctx_.remove(object);
  if (ret < 0) {
    throw RadosRuntimeError(
        "deleting object '" + object + "' from pool '" + pool_ + "' failed",
        ret);
  }
}
