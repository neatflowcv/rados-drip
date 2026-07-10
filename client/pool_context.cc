#include "client/pool_context.h"

#include <cstddef>
#include <optional>
#include <rados/librados.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "client/rados_error.h"

namespace {

constexpr std::size_t kObjectListBatchSize = 1024;

}  // namespace

PoolContext::PoolContext(std::string pool, librados::IoCtx ioctx)
    : pool_(std::move(pool)), ioctx_(std::move(ioctx)) {}

const std::string& PoolContext::Name() const { return pool_; }

ListObjectsResult PoolContext::ListObjects(
    const std::optional<std::string>& cursor) {
  librados::ObjectCursor current = ioctx_.object_list_begin();
  const librados::ObjectCursor end = ioctx_.object_list_end();
  if (cursor && !current.from_str(*cursor)) {
    throw std::runtime_error("invalid cursor: " + *cursor);
  }

  std::vector<librados::ObjectItem> objects;
  librados::ObjectCursor next;
  const int ret = ioctx_.object_list(current, end, kObjectListBatchSize, {},
                                     &objects, &next);
  if (ret < 0) {
    throw client_detail::RadosRuntimeError(
        "listing objects in pool '" + pool_ + "' failed", ret);
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
    throw client_detail::RadosRuntimeError(
        "deleting object '" + object + "' from pool '" + pool_ + "' failed",
        ret);
  }
}
