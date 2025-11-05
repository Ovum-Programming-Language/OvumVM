#include "CacheKey.hpp"

namespace ovum::vm::execution_tree {

bool CacheKey::operator==(const CacheKey& other) const {
  if (values_.size() != other.values_.size() || hash_values_.size() != other.hash_values_.size()) {
    return false;
  }

  for (size_t i = 0; i < values_.size(); ++i) {
    if (values_[i] != other.values_[i]) {
      return false;
    }
  }

  return true;
}

} // namespace ovum::vm::execution_tree
