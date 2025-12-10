#ifndef EXECUTION_TREE_FUNCTIONREPOSITORY_HPP
#define EXECUTION_TREE_FUNCTIONREPOSITORY_HPP

#include <cstddef>
#include <expected>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "IFunctionExecutable.hpp"
#include "lib/runtime/FunctionId.hpp"

namespace ovum::vm::execution_tree {

class IFunctionExecutable;

class FunctionRepository {
public:
  FunctionRepository();

  void Reserve(size_t count);

  [[nodiscard]] std::expected<size_t, std::runtime_error> Add(std::unique_ptr<IFunctionExecutable> function);

  [[nodiscard]] std::expected<IFunctionExecutable*, std::runtime_error> GetByIndex(size_t index) const;

  [[nodiscard]] std::expected<IFunctionExecutable*, std::runtime_error> GetById(const runtime::FunctionId& id) const;

  [[nodiscard]] std::expected<IFunctionExecutable*, std::runtime_error> GetByName(const std::string& name) const;

  [[nodiscard]] size_t GetCount() const;

private:
  std::vector<std::unique_ptr<IFunctionExecutable>> functions_;
  std::unordered_map<runtime::FunctionId, size_t> index_by_id_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_FUNCTIONREPOSITORY_HPP
