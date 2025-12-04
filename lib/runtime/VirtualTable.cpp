#include "VirtualTable.hpp"

#include <utility>

#include "VariableAccessor.hpp"

namespace ovum::vm::runtime {

const std::unordered_map<std::string, std::shared_ptr<IVariableAccessor>> VirtualTable::kVariableAccessorsByTypeName = {
    {"int", std::make_shared<VariableAccessor<int64_t>>()},
    {"float", std::make_shared<VariableAccessor<double>>()},
    {"bool", std::make_shared<VariableAccessor<bool>>()},
    {"char", std::make_shared<VariableAccessor<char>>()},
    {"byte", std::make_shared<VariableAccessor<uint8_t>>()},
    {"Object", std::make_shared<VariableAccessor<void*>>()},
};

VirtualTable::VirtualTable(std::string name, size_t size) : name_(std::move(name)), size_(size) {
}

std::string VirtualTable::GetName() const {
  return name_;
}

size_t VirtualTable::GetSize() const {
  return size_;
}

std::expected<Variable, std::runtime_error> VirtualTable::GetVariableByIndex(void* object_ptr, size_t index) const {
  if (index >= fields_.size()) {
    return std::unexpected{
        std::runtime_error("VTable of class " + name_ + " does not contain field number " + std::to_string(index))};
  }

  const FieldInfo& field_info = fields_[index];

  return field_info.variable_accessor->GetVariable(reinterpret_cast<char*>(object_ptr) + field_info.offset);
}

std::expected<void, std::runtime_error> VirtualTable::SetVariableByIndex(void* object_ptr,
                                                                         size_t index,
                                                                         const Variable& variable) const {
  if (index >= fields_.size()) {
    return std::unexpected{
        std::runtime_error("VTable of class " + name_ + " does not contain field number " + std::to_string(index))};
  }

  const FieldInfo& field_info = fields_[index];

  return field_info.variable_accessor->WriteVariable(reinterpret_cast<char*>(object_ptr) + field_info.offset, variable);
}

std::expected<FunctionId, std::runtime_error> VirtualTable::GetRealFunctionId(
    const FunctionId& virtual_function_id) const {
  auto function_info_it = functions_.find(virtual_function_id);

  if (function_info_it == functions_.end()) {
    return std::unexpected{
        std::runtime_error("VTable of class " + name_ + " does not contain function: " + virtual_function_id)};
  }

  return function_info_it->second;
}

void VirtualTable::AddFunction(const FunctionId& virtual_function_id, const FunctionId& real_function_id) {
  functions_[virtual_function_id] = real_function_id;
}

size_t VirtualTable::AddField(const std::string& type_name, int64_t offset) {
  fields_.emplace_back(FieldInfo{
      .offset = offset,
      .variable_accessor = kVariableAccessorsByTypeName.at(type_name),
  });

  return fields_.size() - 1U;
}

void VirtualTable::AddInterface(const std::string& interface_name) {
  interfaces_.insert(interface_name);
}

bool VirtualTable::IsType(const std::string& interface_name) const {
  return interfaces_.contains(interface_name) || interface_name == name_;
}

} // namespace ovum::vm::runtime
