#include "client/client.h"

#include <rados/librados.hpp>
#include <rados/rados_types.hpp>
#include <string>
#include <utility>

#include "client/pool_context.h"
#include "client/rados_error.h"

namespace {

void InitCluster(librados::Rados& cluster, const std::string& client_name,
                 const std::string& cluster_name) {
  const int ret = cluster.init2(client_name.c_str(), cluster_name.c_str(), 0);
  if (ret < 0) {
    throw client_detail::RadosRuntimeError("rados init failed", ret);
  }
}

void ConnectCluster(librados::Rados& cluster) {
  const int ret = cluster.connect();
  if (ret < 0) {
    throw client_detail::RadosRuntimeError("connecting to Ceph failed", ret);
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
      throw client_detail::RadosRuntimeError("setting mon_host failed", ret);
    }

    ret = cluster_.conf_set("keyring", "");
    if (ret < 0) {
      throw client_detail::RadosRuntimeError("disabling keyring search failed",
                                             ret);
    }

    ret = cluster_.conf_set("key", options.key.c_str());
    if (ret < 0) {
      throw client_detail::RadosRuntimeError("setting CephX key failed", ret);
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
    throw client_detail::RadosRuntimeError("opening pool '" + pool + "' failed",
                                           ret);
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
