#ifndef RUNTIME_OBJECTREPOSITORY_HPP
#define RUNTIME_OBJECTREPOSITORY_HPP

#include <cstddef>
#include <expected>
#include <stdexcept>
#include <vector>

#include "ObjectDescriptor.hpp"

namespace ovum::vm::runtime {

class ObjectRepository {
public:
  ObjectRepository();

  void Reserve(size_t count);

  [[nodiscard]] std::expected<size_t, std::runtime_error> Add(ObjectDescriptor* descriptor);
  std::expected<void, std::runtime_error> Remove(size_t index);
  void Clear();

  [[nodiscard]] std::expected<ObjectDescriptor*, std::runtime_error> GetByIndex(size_t index);

  [[nodiscard]] std::expected<const ObjectDescriptor*, std::runtime_error> GetByIndex(size_t index) const;

  [[nodiscard]] size_t GetCount() const;

private:
  std::vector<ObjectDescriptor*> objects_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_OBJECTREPOSITORY_HPP
