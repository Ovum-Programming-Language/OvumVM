#ifndef RUNTIME_MARKANDSWEEPGC_HPP
#define RUNTIME_MARKANDSWEEPGC_HPP

#include <queue>

#include "lib/runtime/gc/IGarbageCollector.hpp"

namespace ovum::vm::runtime {

class MarkAndSweepGC : public IGarbageCollector {
public:
  std::expected<void, std::runtime_error> Collect(execution_tree::PassedExecutionData& data) override;

private:
  void Mark(execution_tree::PassedExecutionData& data);
  std::expected<void, std::runtime_error> Sweep(execution_tree::PassedExecutionData& data);
  void AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data);
};

} // namespace ovum::vm::runtime

#endif // RUNTIME_MARKANDSWEEPGC_HPP
