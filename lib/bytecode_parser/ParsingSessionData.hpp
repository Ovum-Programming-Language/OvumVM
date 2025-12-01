#pragma once

#include <memory>
#include <optional>

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/executor/IJitExecutorFactory.hpp"
#include "lib/runtime/RuntimeMemory.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::bytecode::parser {

struct ParsingSessionData {
  vm::execution_tree::FunctionRepository& func_repo;
  vm::runtime::VirtualTableRepository& vtable_repo;
  vm::runtime::RuntimeMemory& memory;

  std::unique_ptr<vm::execution_tree::Block> init_static_block;
  vm::execution_tree::Block* current_block = nullptr;

  std::optional<std::reference_wrapper<vm::executor::IJitExecutorFactory>> jit_factory;
  size_t jit_boundary = 0;
};

} // namespace ovum::bytecode::parser
