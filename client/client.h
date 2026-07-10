#pragma once

#include <rados/librados.hpp>
#include <string>

class PoolContext;

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
