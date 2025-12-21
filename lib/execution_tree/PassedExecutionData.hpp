#ifndef EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP
#define EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP

#include <istream>
#include <memory>
#include <ostream>

#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::execution_tree {

class FunctionRepository;

struct PassedExecutionData {
  runtime::RuntimeMemory& memory;
  const runtime::VirtualTableRepository& virtual_table_repository;
  const FunctionRepository& function_repository;
  std::allocator<char> allocator;
  std::istream& input_stream;
  std::ostream& output_stream;
  std::ostream& error_stream;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_PASSEDEXECUTIONDATA_HPP
