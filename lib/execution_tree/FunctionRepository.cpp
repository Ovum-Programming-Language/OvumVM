#include "FunctionRepository.hpp"

#include "IFunctionExecutable.hpp"

namespace ovum::vm::execution_tree {

FunctionRepository::FunctionRepository() = default;

void FunctionRepository::Reserve(size_t count) {
  functions_.reserve(count);
}

std::expected<size_t, std::runtime_error> FunctionRepository::Add(std::unique_ptr<IFunctionExecutable> function) {
  const runtime::FunctionId id = function->GetId();

  if (index_by_id_.find(id) != index_by_id_.end()) {
    return std::unexpected(std::runtime_error("Function with the same id already exists: " + id));
  }

  functions_.emplace_back(std::move(function));
  index_by_id_[id] = functions_.size() - 1U;

  return functions_.size() - 1U;
}

std::expected<IFunctionExecutable*, std::runtime_error> FunctionRepository::GetByIndex(size_t index) const {
  if (index >= functions_.size()) {
    return std::unexpected(std::runtime_error("Function index out of range"));
  }

  return functions_[index].get();
}

std::expected<IFunctionExecutable*, std::runtime_error> FunctionRepository::GetById(const runtime::FunctionId& id) const {
  const auto it = index_by_id_.find(id);

  if (it == index_by_id_.end()) {
    return std::unexpected(std::runtime_error("Function not found by id: " + id));
  }

  return functions_[it->second].get();
}

std::expected<IFunctionExecutable*, std::runtime_error> FunctionRepository::GetByName(const std::string& name) const {
  const auto it = index_by_id_.find(name);

  if (it == index_by_id_.end()) {
    return std::unexpected(std::runtime_error("Function not found by name: " + name));
  }

  return functions_[it->second].get();
}

size_t FunctionRepository::GetCount() const {
  return functions_.size();
}

} // namespace ovum::vm::execution_tree
