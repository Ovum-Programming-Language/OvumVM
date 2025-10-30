#ifndef RUNTIME_RUNTIMEMEMORY_HPP
#define RUNTIME_RUNTIMEMEMORY_HPP

#include <stack>

#include "ObjectRepository.hpp"
#include "Variable.hpp"
#include "VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

struct RuntimeMemory {
  VariableCollection global_variables;
  std::stack<VariableCollection> local_variables;
  VariableStack machine_stack;
  ObjectRepository object_repository;
  VirtualTableRepository virtual_table_repository;
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_RUNTIMEMEMORY_HPP
