#ifndef RUNTIME_RUNTIMEMEMORY_HPP
#define RUNTIME_RUNTIMEMEMORY_HPP

#include <stack>

#include "ObjectRepository.hpp"
#include "StackFrame.hpp"
#include "Variable.hpp"

namespace ovum::vm::runtime {

struct RuntimeMemory {
  VariableCollection global_variables;
  std::stack<StackFrame> stack_frames;
  VariableStack machine_stack;
  ObjectRepository object_repository;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_RUNTIMEMEMORY_HPP
