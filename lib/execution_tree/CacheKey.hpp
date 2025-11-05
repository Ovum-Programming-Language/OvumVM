#ifndef EXECUTION_TREE_CACHEKEY_HPP
#define EXECUTION_TREE_CACHEKEY_HPP

#include <cstddef>
#include <vector>

#include "lib/runtime/Variable.hpp"

namespace ovum::vm::execution_tree {

class CacheKey {
public:
  CacheKey() = default;
  CacheKey(std::vector<runtime::Variable> values, std::vector<size_t> hash_values) :
      values_(std::move(values)), hash_values_(std::move(hash_values)) {
  }

  [[nodiscard]] bool operator==(const CacheKey& other) const;

  [[nodiscard]] const std::vector<runtime::Variable>& GetValues() const {
    return values_;
  }

  [[nodiscard]] std::vector<runtime::Variable>& GetValues() {
    return values_;
  }

  [[nodiscard]] const std::vector<size_t>& GetHashValues() const {
    return hash_values_;
  }

  [[nodiscard]] std::vector<size_t>& GetHashValues() {
    return hash_values_;
  }

private:
  std::vector<runtime::Variable> values_;
  std::vector<size_t> hash_values_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_CACHEKEY_HPP
