#ifndef RUNTIME_OBJECTREPOSITORY_HPP
#define RUNTIME_OBJECTREPOSITORY_HPP

#include <cstddef>
#include <expected>
#include <functional>
#include <stdexcept>
#include <unordered_set>

#include "ObjectDescriptor.hpp"

namespace ovum::vm::runtime {

class ObjectRepository {
public:
  ObjectRepository();

  void Reserve(size_t count);

  [[nodiscard]] std::expected<void, std::runtime_error> Add(ObjectDescriptor* descriptor);
  std::expected<void, std::runtime_error> Remove(ObjectDescriptor* descriptor);
  void Clear();

  void ForAll(const std::function<void(void*)>& func) const;

  [[nodiscard]] size_t GetCount() const;

private:
  std::unordered_set<ObjectDescriptor*> objects_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_OBJECTREPOSITORY_HPP
