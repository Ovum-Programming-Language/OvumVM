#ifndef RUNTIME_VARIABLEACCESSOR_HPP
#define RUNTIME_VARIABLEACCESSOR_HPP

#include <string>
#include <typeinfo>

#include "IVariableAccessor.hpp"

namespace ovum::vm::runtime {

template<VariableMemberType T>
class VariableAccessor : public IVariableAccessor {
public:
  Variable GetVariable(void* value_ptr) const override {
    return {*reinterpret_cast<T*>(value_ptr)};
  }

  std::expected<void, std::runtime_error> WriteVariable(void* value_ptr, const Variable& variable) const override {
    if (!std::holds_alternative<T>(variable)) {
      return std::unexpected{std::runtime_error("Variable type mismatch: expected " + std::string(typeid(T).name()) +
                                                ", got " + std::to_string(variable.index()))};
    }

    *reinterpret_cast<T*>(value_ptr) = std::get<T>(variable);

    return {};
  }

  [[nodiscard]] bool IsReferenceType() const override {
    return std::is_same_v<T, void*>;
  }
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_VARIABLEACCESSOR_HPP
