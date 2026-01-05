#include "Executor.hpp"

#include <vector>

#include "lib/execution_tree/BytecodeCommands.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/runtime/StackFrame.hpp"
#include "lib/runtime/Variable.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::executor {

const std::string Executor::kMainFunctionName = "_Global_Main_StringArray";

Executor::Executor(execution_tree::PassedExecutionData& execution_data) : execution_data_(execution_data) {
}

namespace {
std::expected<void*, std::runtime_error> CreateStringArrayFromArgs(execution_tree::PassedExecutionData& execution_data,
                                                                   const std::vector<std::string>& args) {
  auto string_vtable_result = execution_data.virtual_table_repository.GetByName("String");
  if (!string_vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: String vtable not found"));
  }

  const runtime::VirtualTable* string_vtable = string_vtable_result.value();
  auto string_vtable_index_result = execution_data.virtual_table_repository.GetIndexByName("String");
  if (!string_vtable_index_result.has_value()) {
    return std::unexpected(string_vtable_index_result.error());
  }

  auto default_string_obj_result = execution_data.memory_manager.AllocateObject(
      *string_vtable, static_cast<uint32_t>(string_vtable_index_result.value()), execution_data);

  if (!default_string_obj_result.has_value()) {
    return std::unexpected(default_string_obj_result.error());
  }

  void* default_string_obj = default_string_obj_result.value();
  auto* default_string_data = runtime::GetDataPointer<std::string>(default_string_obj);
  new (default_string_data) std::string();

  execution_data.memory.machine_stack.emplace(default_string_obj);
  execution_data.memory.machine_stack.emplace(static_cast<int64_t>(args.size()));

  auto string_array_result = execution_tree::bytecode::CallConstructor(execution_data, "_StringArray_int_String");
  if (!string_array_result.has_value()) {
    return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: failed to create StringArray: " +
                                              std::string(string_array_result.error().what())));
  }

  if (execution_data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: StringArray not on stack"));
  }
  runtime::Variable string_array_var = execution_data.memory.machine_stack.top();
  execution_data.memory.machine_stack.pop();

  if (!std::holds_alternative<void*>(string_array_var)) {
    return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: StringArray is not an object"));
  }
  void* string_array_obj = std::get<void*>(string_array_var);

  auto set_at_result = execution_data.function_repository.GetByName("_StringArray_SetAt_<M>_int_String");
  if (!set_at_result.has_value()) {
    return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: StringArray SetAt not found"));
  }

  for (size_t i = 0; i < args.size(); ++i) {
    auto string_obj_result = execution_data.memory_manager.AllocateObject(
        *string_vtable, static_cast<uint32_t>(string_vtable_index_result.value()), execution_data);

    if (!string_obj_result.has_value()) {
      return std::unexpected(string_obj_result.error());
    }

    void* string_obj = string_obj_result.value();
    auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
    new (string_data) std::string(args[i]);

    execution_data.memory.machine_stack.emplace(string_obj);
    execution_data.memory.machine_stack.emplace(static_cast<int64_t>(i));
    execution_data.memory.machine_stack.emplace(string_array_obj);

    auto set_at_exec_result = set_at_result.value()->Execute(execution_data);
    if (!set_at_exec_result.has_value()) {
      return std::unexpected(std::runtime_error("CreateStringArrayFromArgs: SetAt execution failed: " +
                                                std::string(set_at_exec_result.error().what())));
    }
  }

  return string_array_obj;
}
} // namespace

std::expected<int64_t, std::runtime_error> Executor::RunProgram(
    const std::unique_ptr<execution_tree::Block>& init_static, const std::vector<std::string>& args) {
  if (!init_static) {
    return std::unexpected(std::runtime_error("Execution failed: init-static block is null"));
  }

  runtime::StackFrame init_frame{};
  execution_data_.memory.stack_frames.push(std::move(init_frame));

  const std::expected<execution_tree::ExecutionResult, std::runtime_error> block_result =
      init_static->Execute(execution_data_);

  execution_data_.memory.stack_frames.pop();

  if (!block_result.has_value()) {
    return std::unexpected(block_result.error());
  }

  const std::expected<execution_tree::IFunctionExecutable*, std::runtime_error> main_function =
      execution_data_.function_repository.GetByName(kMainFunctionName);

  if (!main_function.has_value()) {
    return std::unexpected(std::runtime_error("Execution failed: main function '" + kMainFunctionName + "' not found"));
  }

  auto string_array_result = CreateStringArrayFromArgs(execution_data_, args);

  if (!string_array_result.has_value()) {
    return std::unexpected(std::runtime_error("Execution failed: failed to create StringArray: " +
                                              std::string(string_array_result.error().what())));
  }

  execution_data_.memory.machine_stack.emplace(string_array_result.value());

  auto main_exec_result = main_function.value()->Execute(execution_data_);

  if (!main_exec_result.has_value()) {
    return std::unexpected(main_exec_result.error());
  }

  if (execution_data_.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("Execution failed: main function did not return a value"));
  }

  runtime::Variable return_value = execution_data_.memory.machine_stack.top();
  execution_data_.memory.machine_stack.pop();

  if (!std::holds_alternative<int64_t>(return_value)) {
    return std::unexpected(std::runtime_error("Execution failed: main function did not return an int64_t"));
  }

  return std::get<int64_t>(return_value);
}

} // namespace ovum::vm::executor
