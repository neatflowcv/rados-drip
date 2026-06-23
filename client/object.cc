#include "client/object.h"

#include <utility>

Object::Object(std::string name, std::string object_namespace)
    : name_(std::move(name)), namespace_(std::move(object_namespace)) {}

const std::string& Object::Name() const { return name_; }

const std::string& Object::Namespace() const { return namespace_; }
