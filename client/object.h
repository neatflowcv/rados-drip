#pragma once

#include <string>

class Object {
 public:
  Object(std::string name, std::string object_namespace);

  [[nodiscard]] const std::string& Name() const;
  [[nodiscard]] const std::string& Namespace() const;

 private:
  std::string name_;
  std::string namespace_;
};
