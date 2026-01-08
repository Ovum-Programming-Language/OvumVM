#include "MarkAndSweepGC.hpp"

#include <optional>
#include <queue>
#include <stack>
#include <vector>

#include "lib/execution_tree/PassedExecutionData.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

std::expected<void, std::runtime_error> MarkAndSweepGC::Collect(execution_tree::PassedExecutionData& data) {
  Mark(data);
  return Sweep(data);
}

void MarkAndSweepGC::Mark(execution_tree::PassedExecutionData& data) {
  std::queue<void*> worklist;
  AddRoots(worklist, data);

  while (!worklist.empty()) {
    void* obj = worklist.front();
    worklist.pop();

    if (obj == nullptr) {
      continue;
    }

    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);
    if (desc->badge & kMarkBit) {
      continue;
    }
    desc->badge |= kMarkBit;

    std::expected<const VirtualTable*, std::runtime_error> vt_res =
        data.virtual_table_repository.GetByIndex(desc->vtable_index);

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

std::expected<void, std::runtime_error> MarkAndSweepGC::Sweep(execution_tree::PassedExecutionData& data) {
  std::vector<void*> to_delete;
  const ObjectRepository& repo = data.memory_manager.GetRepository();

  repo.ForAll([&to_delete](void* obj) {
    auto* desc = reinterpret_cast<ObjectDescriptor*>(obj);

    if (!(desc->badge & kMarkBit)) {
      to_delete.push_back(obj);
    }

    desc->badge &= ~kMarkBit;
  });

  std::optional<std::runtime_error> first_error;

  for (void* obj : to_delete) {
    std::expected<void, std::runtime_error> dealloc_res = data.memory_manager.DeallocateObject(obj, data);

    if (!dealloc_res.has_value() && !first_error) {
      first_error = dealloc_res.error();
    }
  }

  if (first_error) {
    return std::unexpected(*first_error);
  }

  return {};
}

void MarkAndSweepGC::AddRoots(std::queue<void*>& worklist, execution_tree::PassedExecutionData& data) {
  for (const Variable& var : data.memory.global_variables) {
    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);

      if (ptr) {
        worklist.push(ptr);
      }
    }
  }

  // Note that there is no way to traverse a std::stack without emptying it, so we need to create a temporary stack.
  std::stack<StackFrame> temp_stack_frames = data.memory.stack_frames;

  while (!temp_stack_frames.empty()) {
    const StackFrame& frame = temp_stack_frames.top();

    for (const Variable& var : frame.local_variables) {
      if (std::holds_alternative<void*>(var)) {
        void* ptr = std::get<void*>(var);

        if (ptr) {
          worklist.push(ptr);
        }
      }
    }

    temp_stack_frames.pop();
  }

  std::stack<Variable> temp_machine_stack = data.memory.machine_stack;

  while (!temp_machine_stack.empty()) {
    const Variable& var = temp_machine_stack.top();

    if (std::holds_alternative<void*>(var)) {
      void* ptr = std::get<void*>(var);

      if (ptr) {
        worklist.push(ptr);
      }
    }

    temp_machine_stack.pop();
  }
}

} // namespace ovum::vm::runtime
