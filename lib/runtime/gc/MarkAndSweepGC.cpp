#include "MarkAndSweepGC.hpp"

#include <queue>
#include <ranges>
#include <stack>
#include <vector>

#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

std::expected<void, std::runtime_error> MarkAndSweepGC::Collect(execution_tree::PassedExecutionData& data) {
  Mark(data);
  Sweep(data);
  return {};
}

void MarkAndSweepGC::Mark(execution_tree::PassedExecutionData& data) {
  std::queue<void*> worklist;
  AddRoots(worklist, data);

  while (!worklist.empty()) {
    void* obj = worklist.front();
    worklist.pop();

    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);
    if (desc->badge & kMarkBit) {
      continue;
    }
    desc->badge |= kMarkBit;

    auto vt_res = data.virtual_table_repository.GetByIndex(desc->vtable_index);
    if (!vt_res.has_value()) {
      continue;
    }
    const VirtualTable* vt = vt_res.value();

    vt->ScanReferences(obj, [&](void* ref) {
      if (ref) {
        worklist.push(ref);
      }
    });
  }
}

void MarkAndSweepGC::Sweep(execution_tree::PassedExecutionData& data) {
  std::vector<void*> to_delete;
  to_delete.reserve(data.memory_manager.GetRepository().GetCount() / 4);

  const ObjectRepository& repo = data.memory_manager.GetRepository();
  for (size_t i = 0; i < repo.GetCount(); ++i) {
    auto obj_res = repo.GetByIndex(i);
    if (!obj_res.has_value()) {
      continue;
    }

    const ObjectDescriptor* const_desc = obj_res.value();
    void* obj = const_cast<void*>(static_cast<const void*>(const_desc));

    if (!(const_desc->badge & kMarkBit)) {
      to_delete.push_back(obj);
    }

    const_cast<ObjectDescriptor*>(const_desc)->badge &= ~kMarkBit;
  }

  for (auto obj : std::ranges::reverse_view(to_delete)) {
    auto dealloc_res = data.memory_manager.DeallocateObject(obj, data);
  }
}

void MarkAndSweepGC::AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data) {
  for (const auto& var : data.memory.global_variables) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
      }
    }
  }

  std::stack<StackFrame> temp_frames = data.memory.stack_frames;

  while (!temp_frames.empty()) {
    const StackFrame& frame = temp_frames.top();
    for (const auto& var : frame.local_variables) {
      if (std::holds_alternative<void*>(var)) {
        void* ptr = std::get<void*>(var);
        if (ptr) {
          worklist.push(ptr);
        }
      }
    }
    temp_frames.pop();
  }

  std::vector<Variable> temp_stack;
  temp_stack.reserve(data.memory.machine_stack.size());

  std::stack<Variable> temp_machine_stack = data.memory.machine_stack;
  while (!temp_machine_stack.empty()) {
    temp_stack.push_back(temp_machine_stack.top());
    temp_machine_stack.pop();
  }

  for (const auto& var : temp_stack) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);
      if (ptr) {
        worklist.push(ptr);
      }
    }
  }
}

} // namespace ovum::vm::runtime
