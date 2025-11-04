#ifndef RUNTIME_VIRTUAL_TABLE_HPP
#define RUNTIME_VIRTUAL_TABLE_HPP

#include <expected>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "FieldInfo.hpp"
#include "FunctionId.hpp"
#include "IVariableAccessor.hpp"
#include "Variable.hpp"

namespace ovum::vm::runtime {

class VirtualTable {
public:
  VirtualTable(std::string name, size_t size);

  [[nodiscard]] std::string GetName() const;
  [[nodiscard]] size_t GetSize() const;

  [[nodiscard]] std::expected<Variable, std::runtime_error> GetVariableByIndex(void* object_ptr, size_t index) const;
  std::expected<void, std::runtime_error> SetVariableByIndex(void* object_ptr,
                                                             size_t index,
                                                             const Variable& variable) const;
  [[nodiscard]] std::expected<FunctionId, std::runtime_error> GetRealFunctionId(
      const FunctionId& virtual_function_id) const;

  void AddFunction(const FunctionId& virtual_function_id, const FunctionId& real_function_id);
  size_t AddField(const std::string& type_name, int64_t offset);

private:
  static const std::unordered_map<std::string, std::shared_ptr<IVariableAccessor>> kVariableAccessorsByTypeName;

  std::string name_;
  size_t size_;
  std::vector<FieldInfo> fields_;
  std::unordered_map<FunctionId, FunctionId> functions_;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_VIRTUAL_TABLE_HPP
