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

class Client {
 public:
  Client(const std::optional<std::string>& conf_path,
         const std::optional<std::string>& keyring_path,
         const std::string& client_name, const std::string& cluster_name);
  ~Client();

  ListObjectsResult ListObjects(const std::string& pool,
                                const std::optional<std::string>& cursor);

  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  Client(Client&&) = delete;
  Client& operator=(Client&&) = delete;

 private:
  librados::Rados cluster_;
  bool connected_ = false;
};
