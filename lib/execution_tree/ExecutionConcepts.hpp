#ifndef EXECUTION_TREE_EXECUTIONCONCEPTS_HPP
#define EXECUTION_TREE_EXECUTIONCONCEPTS_HPP

#include <expected>
#include <stdexcept>

#include "ExecutionResult.hpp"
#include "IExecutable.hpp"
#include "PassedExecutionData.hpp"

namespace ovum::vm::execution_tree {

template<typename T>
concept Executable = requires(T t, PassedExecutionData& execution_data) {
  { t.Execute(execution_data) } -> std::same_as<std::expected<ExecutionResult, std::runtime_error>>;
};

template<typename T>
concept ExecutableBlock = Executable<T> && requires(T t, std::unique_ptr<IExecutable> statement) {
  { t.AddStatement(statement) } -> std::same_as<void>;
};

template<typename T>
concept ExecutableFunction = Executable<T> && requires(const T& t) {
  { t.GetId() } -> std::same_as<runtime::FunctionId>;
  { t.GetArity() } -> std::same_as<size_t>;
  { t.GetTotalActionCount() } -> std::same_as<size_t>;
  { t.GetExecutionCount() } -> std::same_as<size_t>;
};

template<typename Func>
concept CommandFunction = requires(const Func func, PassedExecutionData& execution_data) {
  { func(execution_data) } -> std::same_as<std::expected<ExecutionResult, std::runtime_error>>;
};

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_EXECUTIONCONCEPTS_HPP
