#include "ObjectRepository.hpp"

namespace ovum::vm::runtime {

ObjectRepository::ObjectRepository() = default;

void ObjectRepository::Reserve(size_t count) {
  objects_.reserve(count);
}

std::expected<size_t, std::runtime_error> ObjectRepository::Add(const ObjectDescriptor& descriptor) {
  objects_.push_back(descriptor);
  return objects_.size() - 1U;
}

std::expected<ObjectDescriptor*, std::runtime_error> ObjectRepository::GetByIndex(size_t index) {
  if (index >= objects_.size()) {
    return std::unexpected(std::runtime_error("ObjectDescriptor index out of range"));
  }

  return &objects_[index];
}

std::expected<const ObjectDescriptor*, std::runtime_error> ObjectRepository::GetByIndex(size_t index) const {
  if (index >= objects_.size()) {
    return std::unexpected(std::runtime_error("ObjectDescriptor index out of range"));
  }

  return &objects_[index];
}

size_t ObjectRepository::GetCount() const {
  return objects_.size();
}

} // namespace ovum::vm::runtime
