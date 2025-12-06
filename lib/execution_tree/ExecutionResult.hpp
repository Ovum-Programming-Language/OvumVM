#ifndef EXECUTION_TREE_EXECUTIONRESULT_HPP
#define EXECUTION_TREE_EXECUTIONRESULT_HPP

namespace ovum::vm::execution_tree {

enum class ExecutionResult { kNormal = 0, kBreak = 1, kContinue = 2, kReturn = 3, kConditionFalse = 4 };

} // namespace ovum::vm::execution_tree

#endif // EXECUTION_TREE_EXECUTIONRESULT_HPP
