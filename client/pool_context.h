#pragma once

#include <optional>
#include <rados/librados.hpp>
#include <string>
#include <vector>

#include "client/object.h"

struct ListObjectsResult {
  std::vector<Object> objects;
  std::string next_cursor;
  bool is_end = false;
};

class PoolContext {
 public:
  [[nodiscard]] const std::string& Name() const;
  ListObjectsResult ListObjects(const std::optional<std::string>& cursor);
  void DeleteObject(const std::string& object);

  PoolContext(const PoolContext&) = delete;
  PoolContext& operator=(const PoolContext&) = delete;
  PoolContext(PoolContext&&) = default;
  PoolContext& operator=(PoolContext&&) = default;

 private:
  friend class Client;

  PoolContext(std::string pool, librados::IoCtx ioctx);

  std::string pool_;
  librados::IoCtx ioctx_;
};
