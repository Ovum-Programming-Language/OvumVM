#ifndef EXECUTION_TREE_COMMAND_HPP
#define EXECUTION_TREE_COMMAND_HPP

#include <expected>
#include <stdexcept>
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

    return func_(execution_data);
  }

private:
  const Func func_;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_COMMAND_HPP
