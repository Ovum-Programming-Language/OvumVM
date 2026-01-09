#ifndef RUNTIME_MARKANDSWEEPGC_HPP
#define RUNTIME_MARKANDSWEEPGC_HPP

#include <queue>
#include <vector>

#include "lib/runtime/Variable.hpp"
#include "lib/runtime/gc/IGarbageCollector.hpp"

namespace ovum::vm::runtime {

class MarkAndSweepGC : public IGarbageCollector {
public:
  std::expected<void, std::runtime_error> Collect(execution_tree::PassedExecutionData& data) override;

private:
  static void Mark(execution_tree::PassedExecutionData& data);
  static std::expected<void, std::runtime_error> Sweep(execution_tree::PassedExecutionData& data);

  static void AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data);
  static void AddAllVariables(std::queue<void*>& worklist, const std::vector<Variable>& variables);
  static void AddAllVariables(std::queue<void*>& worklist, VariableStack variables);
  static void AddRoot(std::queue<void*>& worklist, const Variable& var);
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_MARKANDSWEEPGC_HPP
