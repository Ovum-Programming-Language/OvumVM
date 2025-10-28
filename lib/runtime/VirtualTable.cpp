#include "VirtualTable.hpp"

#include <utility>

#include "VariableAccessor.hpp"

namespace ovum::vm::runtime {

const std::unordered_map<std::string, std::shared_ptr<IVariableAccessor>>
    VirtualTable::variable_accessors_by_type_name_ = {
        {"int", std::make_shared<VariableAccessor<int64_t>>()},
        {"float", std::make_shared<VariableAccessor<double>>()},
        {"bool", std::make_shared<VariableAccessor<bool>>()},
        {"char", std::make_shared<VariableAccessor<char>>()},
        {"byte", std::make_shared<VariableAccessor<uint8_t>>()},
        {"Object", std::make_shared<VariableAccessor<void*>>()},
};

VirtualTable::VirtualTable(std::string name, size_t size) : name_(std::move(name)), size_(size) {
}

std::string VirtualTable::name() const {
  return name_;
}

size_t VirtualTable::size() const {
  return size_;
}

std::expected<Variable, std::runtime_error> VirtualTable::GetVariableByName(void* object_ptr, const std::string& name) const {
  auto field_info_it = fields_.find(name);
  
  if (field_info_it == fields_.end()) {
    return std::unexpected{std::runtime_error("VTable of class " + name_ + " does not contain field: " + name)};
  }

  const FieldInfo& field_info = field_info_it->second;
  
  return field_info.variable_accessor->GetVariable(reinterpret_cast<char*>(object_ptr) + field_info.offset);
}

std::expected<void, std::runtime_error> VirtualTable::SetVariableByName(void* object_ptr, const std::string& name, const Variable& variable) const {
  auto field_info_it = fields_.find(name);
  
  if (field_info_it == fields_.end()) {
    return std::unexpected{std::runtime_error("VTable of class " + name_ + " does not contain field: " + name)};
  }

  const FieldInfo& field_info = field_info_it->second;
  
  return field_info.variable_accessor->WriteVariable(object_ptr, variable);
}

std::expected<FunctionId, std::runtime_error> VirtualTable::GetRealFunctionId(const FunctionId& virtual_function_id) const {
  auto function_info_it = functions_.find(virtual_function_id);
  
  if (function_info_it == functions_.end()) {
    return std::unexpected{std::runtime_error("VTable of class " + name_ + " does not contain function: " + virtual_function_id)};
  }

  return function_info_it->second;
}

void VirtualTable::AddFunction(const FunctionId& virtual_function_id, const FunctionId& real_function_id) {
  functions_[virtual_function_id] = real_function_id;
}

void VirtualTable::AddField(const std::string& name, const std::string& type_name, int64_t offset) {
  fields_[name] = FieldInfo{offset, variable_accessors_by_type_name_.at(type_name)};
}

} // namespace ovum::vm::runtime
