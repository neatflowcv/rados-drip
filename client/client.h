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

struct InlineConnectionOptions {
  std::string host;
  std::string key;
};

class ObjectNamespace {
 public:
  ObjectNamespace() = default;
  explicit ObjectNamespace(std::string value);

  static ObjectNamespace All();
  [[nodiscard]] const std::string& Value() const;

 private:
  std::string value_;
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

class Client {
 public:
  Client(const InlineConnectionOptions& options, const std::string& client_name,
         const std::string& cluster_name);
  ~Client();

  PoolContext OpenPool(const std::string& pool,
                       const ObjectNamespace& object_namespace = {});

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  Client(Client&&) = delete;
  Client& operator=(Client&&) = delete;

 private:
  librados::Rados cluster_;
  bool connected_ = false;
};
