#ifndef RUNTIME_STACKFRAME_HPP
#define RUNTIME_STACKFRAME_HPP

#include <string>

#include "Variable.hpp"

namespace ovum::vm::runtime {

struct StackFrame {
  std::string function_name;
  VariableCollection local_variables;
  size_t action_count{};
};

} // namespace ovum::vm::runtime
#endif // RUNTIME_STACKFRAME_HPP
