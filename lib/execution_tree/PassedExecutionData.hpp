#ifndef EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP
#define EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP

#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::execution_tree {

struct PassedExecutionData {
  runtime::RuntimeMemory& memory;
  const runtime::VirtualTableRepository& virtual_table_repository;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP