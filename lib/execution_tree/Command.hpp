#ifndef EXECUTION_TREE_COMMAND_HPP
#define EXECUTION_TREE_COMMAND_HPP

#include <expected>
#include <stdexcept>
#include <string>
#include <utility>

#include "ExecutionConcepts.hpp"
#include "ExecutionResult.hpp"
#include "IExecutable.hpp"
#include "PassedExecutionData.hpp"

namespace ovum::vm::execution_tree {

template<CommandFunction Func>
class Command : public IExecutable {
public:
  explicit Command(Func func) : func_(std::move(func)) {
  }

  std::expected<ExecutionResult, std::runtime_error> Execute(PassedExecutionData& execution_data) override {
    if (execution_data.memory.stack_frames.empty()) {
      return std::unexpected(std::runtime_error("Command::Execute: stack_frames is empty"));
    }

    ++execution_data.memory.stack_frames.top().action_count;

    auto result = func_(execution_data);

    if (!result.has_value()) {
      std::string error_message = result.error().what();
      error_message += "\nAt function ";
      error_message += execution_data.memory.stack_frames.top().function_name;
      return std::unexpected(std::runtime_error(error_message));
    }

    auto gc_res = execution_data.memory_manager.CollectGarbageIfRequired(execution_data);

    if (!gc_res) {
      return std::unexpected(gc_res.error());
    }

    return result.value();
  }

private:
  const Func func_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_COMMAND_HPP
