#ifndef EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP
#define EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP

#include <memory>

#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::execution_tree {

class FunctionRepository;

struct PassedExecutionData {
  runtime::RuntimeMemory& memory;
  const runtime::VirtualTableRepository& virtual_table_repository;
  const FunctionRepository& function_repository;
  std::allocator<char> allocator;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP
