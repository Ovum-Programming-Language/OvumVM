#include "ObjectRepository.hpp"

namespace ovum::vm::runtime {

ObjectRepository::ObjectRepository() = default;

void ObjectRepository::Reserve(size_t count) {
  objects_.reserve(count);
}

std::expected<void, std::runtime_error> ObjectRepository::Add(ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return std::unexpected(std::runtime_error("ObjectRepository: Cannot add null descriptor"));
  }

  objects_.insert(descriptor);
  return {};
}

std::expected<void, std::runtime_error> ObjectRepository::Remove(ObjectDescriptor* descriptor) {
  if (!descriptor) {
    return std::unexpected(std::runtime_error("ObjectRepository: Cannot remove null descriptor"));
  }

  auto it = objects_.find(descriptor);
  if (it == objects_.end()) {
    return std::unexpected(std::runtime_error("ObjectRepository: Descriptor not found"));
  }

  objects_.erase(it);
  return {};
}

void ObjectRepository::Clear() {
  objects_.clear();
}

void ObjectRepository::ForAll(const std::function<void(void*)>& func) const {
  for (ObjectDescriptor* desc : objects_) {
    func(reinterpret_cast<void*>(desc));
  }
}

size_t ObjectRepository::GetCount() const {
  return objects_.size();
}

} // namespace ovum::vm::runtime
