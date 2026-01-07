#include "BytecodeCommands.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <random>
#include <ranges>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "FunctionRepository.hpp"
#include "IFunctionExecutable.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/runtime/ByteArray.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"

#ifdef _WIN32
#include <windows.h>

#include <psapi.h>
#elif __APPLE__
#include <dlfcn.h>
#include <unistd.h>

#include <mach/mach.h>
#include <mach/task_info.h>
#else
#include <dlfcn.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace ovum::vm::execution_tree::bytecode {

static std::mt19937_64 runtime_random_engine(std::random_device{}()); // NOLINT

template<typename ArgumentType>
std::expected<ArgumentType, std::runtime_error> TryExtractArgument(PassedExecutionData& data,
                                                                   const std::string& function_name) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error(function_name + ": not enough arguments on the stack"));
  }

  runtime::Variable var_argument = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  if (!std::holds_alternative<ArgumentType>(var_argument)) {
    data.memory.machine_stack.emplace(var_argument);
    return std::unexpected(std::runtime_error(function_name + ": variable on the top of the stack has incorrect type"));
  }

  return std::get<ArgumentType>(var_argument);
}

template<typename ArgumentOneType, typename ArgumentTwoType>
std::expected<std::pair<ArgumentOneType, ArgumentTwoType>, std::runtime_error> TryExtractTwoArguments(
    PassedExecutionData& data, const std::string& function_name) {
  auto argument_one = TryExtractArgument<ArgumentOneType>(data, function_name);
  if (!argument_one) {
    return std::unexpected(argument_one.error());
  }

  auto argument_two = TryExtractArgument<ArgumentTwoType>(data, function_name);
  if (!argument_two) {
    data.memory.machine_stack.emplace(*argument_one);
    return std::unexpected(argument_two.error());
  }

  return std::pair<ArgumentOneType, ArgumentTwoType>(*argument_one, *argument_two);
}

std::expected<ExecutionResult, std::runtime_error> PushInt(PassedExecutionData& data, int64_t value) {
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushFloat(PassedExecutionData& data, double value) {
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushBool(PassedExecutionData& data, bool value) {
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushChar(PassedExecutionData& data, char value) {
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushByte(PassedExecutionData& data, uint8_t value) {
  data.memory.machine_stack.emplace(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushString(PassedExecutionData& data, const std::string& value) {
  auto vtable_result = data.virtual_table_repository.GetByName("String");

  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("PushString: String vtable not found"));
  }

  const runtime::VirtualTable* string_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("String");

  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto string_obj_result =
      data.memory_manager.AllocateObject(*string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data);

  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }

  void* string_obj = string_obj_result.value();
  auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(value);
  data.memory.machine_stack.emplace(string_obj);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushNull(PassedExecutionData& data) {
  auto vtable_result = data.virtual_table_repository.GetByName("Nullable");

  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("PushNull: Nullable vtable not found"));
  }

  const runtime::VirtualTable* null_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("Nullable");

  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto null_obj_result =
      data.memory_manager.AllocateObject(*null_vtable, static_cast<uint32_t>(vtable_index_result.value()), data);

  if (!null_obj_result.has_value()) {
    return std::unexpected(null_obj_result.error());
  }

  void* null_obj = null_obj_result.value();
  auto* null_data = runtime::GetDataPointer<void*>(null_obj);
  *null_data = nullptr;
  data.memory.machine_stack.emplace(null_obj);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Pop(PassedExecutionData& data) {
  data.memory.machine_stack.pop();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Dup(PassedExecutionData& data) {
  data.memory.machine_stack.emplace(data.memory.machine_stack.top());

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Swap(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("Swap: not enough arguments on the stack"));
  }

  runtime::Variable var_argument1 = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  if (data.memory.machine_stack.empty()) {
    data.memory.machine_stack.emplace(var_argument1);
    return std::unexpected(std::runtime_error("Swap: not enough arguments on the stack"));
  }

  runtime::Variable var_argument2 = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  data.memory.machine_stack.emplace(var_argument1);
  data.memory.machine_stack.emplace(var_argument2);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Rotate(PassedExecutionData& data, int64_t n) {
  if (n <= 0) {
    return std::unexpected(std::runtime_error("Rotate: n must be greater than 0"));
  }

  if (n == 1) {
    return ExecutionResult::kNormal;
  }

  if (n > data.memory.machine_stack.size()) {
    return std::unexpected(std::runtime_error("Rotate: n is greater than the size of the stack"));
  }

  std::vector<runtime::Variable> temp;
  temp.reserve(static_cast<size_t>(n));

  for (int64_t i = 0; i < n - 1; ++i) {
    temp.push_back(data.memory.machine_stack.top());
    data.memory.machine_stack.pop();
  }

  for (auto& it : std::ranges::reverse_view(temp)) {
    data.memory.machine_stack.emplace(it);
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> LoadLocal(PassedExecutionData& data, size_t index) {
  data.memory.machine_stack.emplace(data.memory.stack_frames.top().local_variables[index]);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetLocal(PassedExecutionData& data, size_t index) {
  if (data.memory.stack_frames.empty()) {
    return std::unexpected(std::runtime_error("SetLocal: stack_frames is empty"));
  }

  if (index >= data.memory.stack_frames.top().local_variables.size()) {
    data.memory.stack_frames.top().local_variables.resize(index + 1);
  }

  data.memory.stack_frames.top().local_variables[index] = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> LoadStatic(PassedExecutionData& data, size_t index) {
  data.memory.machine_stack.emplace(data.memory.global_variables[index]);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetStatic(PassedExecutionData& data, size_t index) {
  if (index >= data.memory.global_variables.size()) {
    data.memory.global_variables.resize(index + 1);
  }

  data.memory.global_variables[index] = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ArrayGet(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ArraySet(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first + arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first - arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first * arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntDivide(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntDivide");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  if (arguments->second == 0) {
    return std::unexpected(std::runtime_error("IntDivide: division by zero"));
  }

  data.memory.machine_stack.emplace(arguments->first / arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntModulo(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntModulo");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  if (arguments->second == 0) {
    return std::unexpected(std::runtime_error("IntModulo: division by zero"));
  }

  data.memory.machine_stack.emplace(arguments->first % arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(-(*argument));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntIncrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntIncrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(*argument + 1);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntDecrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntDecrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(*argument - 1);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first + arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first - arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first * arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatDivide(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatDivide");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  if (arguments->second == 0.0) {
    return std::unexpected(std::runtime_error("FloatDivide: division by zero"));
  }

  data.memory.machine_stack.emplace(arguments->first / arguments->second);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(-(*argument));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatSqrt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatSqrt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  if (*argument < 0) {
    return std::unexpected(std::runtime_error("FloatSqrt: negative argument"));
  }

  data.memory.machine_stack.emplace(std::sqrt(*argument));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first + arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first - arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first * arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteDivide(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteDivide");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  if (arguments->second == 0) {
    return std::unexpected(std::runtime_error("ByteDivide: division by zero"));
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first / arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteModulo(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteModulo");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  if (arguments->second == 0) {
    return std::unexpected(std::runtime_error("ByteModulo: division by zero"));
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first % arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(-(*argument)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteIncrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteIncrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(*argument + 1));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteDecrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteDecrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(*argument - 1));

  return ExecutionResult::kNormal;
}
std::expected<ExecutionResult, std::runtime_error> IntEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first == arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first != arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first < arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first <= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first > arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first >= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first == arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first != arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first < arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first <= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first > arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first >= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first == arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first != arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first < arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first <= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first > arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first >= arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first && arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first || arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<bool>(data, "BoolNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(!(*argument));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first != arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first & arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first | arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first ^ arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(~(*argument));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLeftShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLeftShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first << arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntRightShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntRightShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(arguments->first >> arguments->second);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first & arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first | arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first ^ arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(~(*argument)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLeftShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLeftShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first << arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteRightShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteRightShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(arguments->first >> arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringConcat(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "StringConcat");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  void* string_obj1 = arguments.value().first;
  auto* str1_ptr = runtime::GetDataPointer<std::string>(string_obj1);
  void* string_obj2 = arguments.value().second;
  auto* str2_ptr = runtime::GetDataPointer<std::string>(string_obj2);
  auto push_result = PushString(data, "");
  if (!push_result) {
    return std::unexpected(push_result.error());
  }

  auto string_obj = data.memory.machine_stack.top();
  if (!std::holds_alternative<void*>(string_obj)) {
    return std::unexpected(std::runtime_error("StringConcat: variable on the top of the stack has incorrect type"));
  }

  auto res_ptr = runtime::GetDataPointer<std::string>(std::get<void*>(string_obj));

  res_ptr->append(*str1_ptr);
  res_ptr->append(*str2_ptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringLength");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  data.memory.machine_stack.emplace(static_cast<int64_t>(str_ptr->length()));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringSubstring(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringSubstring");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "StringSubstring");
  if (!arguments) {
    data.memory.machine_stack.emplace(argument.value());
    return std::unexpected(arguments.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);
  auto push_result = PushString(data, "");
  if (!push_result) {
    return std::unexpected(push_result.error());
  }

  auto string_obj = data.memory.machine_stack.top();
  if (!std::holds_alternative<void*>(string_obj)) {
    return std::unexpected(std::runtime_error("StringConcat: variable on the top of the stack has incorrect type"));
  }

  auto res_ptr = runtime::GetDataPointer<std::string>(std::get<void*>(string_obj));
  res_ptr->append(str_ptr->substr(arguments.value().first, arguments.value().second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringCompare(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "StringCompare");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  void* string_obj1 = arguments.value().first;
  auto* str1_ptr = runtime::GetDataPointer<std::string>(string_obj1);
  void* string_obj2 = arguments.value().second;
  auto* str2_ptr = runtime::GetDataPointer<std::string>(string_obj2);

  auto res = std::strcmp(str1_ptr->c_str(), str2_ptr->c_str());

  data.memory.machine_stack.emplace(static_cast<int64_t>(res));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  long long res = std::stoll(*str_ptr);

  data.memory.machine_stack.emplace(static_cast<int64_t>(res));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringToFloat(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringToFloat");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  auto res = std::stod(*str_ptr);

  data.memory.machine_stack.emplace(static_cast<double>(res));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntToString(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntToString");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  return PushString(data, std::to_string(argument.value()));
}

std::expected<ExecutionResult, std::runtime_error> FloatToString(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatToString");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  return PushString(data, std::to_string(argument.value()));
}

std::expected<ExecutionResult, std::runtime_error> IntToFloat(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntToFloat");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<double>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<int64_t>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<int64_t>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> CharToByte(PassedExecutionData& data) {
  auto argument = TryExtractArgument<char>(data, "CharToByte");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteToChar(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteToChar");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<char>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolToByte(PassedExecutionData& data) {
  auto argument = TryExtractArgument<bool>(data, "BoolToByte");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.emplace(static_cast<uint8_t>(argument.value()));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Call(PassedExecutionData& data, const std::string& function_name) {
  auto function = data.function_repository.GetByName(function_name);

  if (!function) {
    return std::unexpected(function.error());
  }

  return function.value()->Execute(data);
}

std::expected<ExecutionResult, std::runtime_error> CallIndirect(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "CallIndirect");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto function = data.function_repository.GetByIndex(static_cast<size_t>(argument.value()));

  if (!function) {
    return std::unexpected(function.error());
  }

  return function.value()->Execute(data);
}

std::expected<ExecutionResult, std::runtime_error> CallVirtual(PassedExecutionData& data, const std::string& method) {
  auto argument = TryExtractArgument<void*>(data, "CallVirtual");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto vtable = data.virtual_table_repository.GetByIndex(
      static_cast<ovum::vm::runtime::ObjectDescriptor*>(argument.value())->vtable_index);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto function_id = vtable.value()->GetRealFunctionId(method);

  if (!function_id) {
    return std::unexpected(function_id.error());
  }

  const runtime::FunctionId const_function_id = std::move(function_id.value());
  auto function = data.function_repository.GetById(const_function_id);

  if (!function) {
    return std::unexpected(function.error());
  }

  data.memory.machine_stack.emplace(argument.value());

  return function.value()->Execute(data);
}

std::expected<ExecutionResult, std::runtime_error> Return(PassedExecutionData& data) {
  return ExecutionResult::kReturn;
}

std::expected<ExecutionResult, std::runtime_error> Break(PassedExecutionData& data) {
  return ExecutionResult::kBreak;
}

std::expected<ExecutionResult, std::runtime_error> Continue(PassedExecutionData& data) {
  return ExecutionResult::kContinue;
}

std::expected<ExecutionResult, std::runtime_error> GetField(PassedExecutionData& data, size_t number) {
  auto argument = TryExtractArgument<void*>(data, "GetField");

  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto vtable = data.virtual_table_repository.GetByIndex(
      reinterpret_cast<runtime::ObjectDescriptor*>(argument.value())->vtable_index);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto field = vtable.value()->GetVariableByIndex(argument.value(), number);

  if (!field) {
    return std::unexpected(field.error());
  }

  data.memory.machine_stack.emplace(field.value());

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetField(PassedExecutionData& data, size_t number) {
  auto argument1 = TryExtractArgument<void*>(data, "SetField");

  if (!argument1) {
    return std::unexpected(argument1.error());
  }

  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("SetField: not enough arguments on the stack"));
  }

  runtime::Variable argument2 = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  auto vtable = data.virtual_table_repository.GetByIndex(
      reinterpret_cast<runtime::ObjectDescriptor*>(argument1.value())->vtable_index);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto result = vtable.value()->SetVariableByIndex(argument1.value(), number, argument2);

  if (!result) {
    return std::unexpected(result.error());
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> CallConstructor(PassedExecutionData& data,
                                                                   const std::string& constructor_name) {
  std::string class_name;
  size_t first_underscore = constructor_name.find('_');
  size_t second_underscore = constructor_name.find('_', first_underscore + 1);

  if (first_underscore != std::string::npos && second_underscore != std::string::npos &&
      second_underscore > first_underscore + 1) {
    class_name = constructor_name.substr(first_underscore + 1, second_underscore - first_underscore - 1);
  } else {
    class_name = constructor_name;
  }

  auto vtable_idx = data.virtual_table_repository.GetIndexByName(class_name);

  if (!vtable_idx) {
    return std::unexpected(vtable_idx.error());
  }

  auto vtable = data.virtual_table_repository.GetByIndex(vtable_idx.value());

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto obj_ptr = data.memory_manager.AllocateObject(*vtable.value(), vtable_idx.value(), data);

  if (!obj_ptr) {
    return std::unexpected(obj_ptr.error());
  }

  auto ctor = data.function_repository.GetByName(constructor_name);

  if (!ctor) {
    return std::unexpected(ctor.error());
  }

  data.memory.machine_stack.emplace(obj_ptr.value());

  return ctor.value()->Execute(data);
}

std::expected<ExecutionResult, std::runtime_error> Unwrap(PassedExecutionData& data) {
  auto wrapper_result = TryExtractArgument<void*>(data, "Unwrap");

  if (!wrapper_result) {
    return std::unexpected(wrapper_result.error());
  }

  auto object_descriptor_ptr = reinterpret_cast<runtime::ObjectDescriptor*>(wrapper_result.value());
  auto vtable_idx = object_descriptor_ptr->vtable_index;
  auto vtable = data.virtual_table_repository.GetByIndex(vtable_idx);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto wrapped_result = vtable.value()->GetVariableByIndex(wrapper_result.value(), 0);

  if (!wrapped_result) {
    return std::unexpected(wrapped_result.error());
  }

  runtime::Variable wrapped = wrapped_result.value();

  if (std::holds_alternative<void*>(wrapped) && std::get<void*>(wrapped) == nullptr) {
    return std::unexpected(std::runtime_error("Unwrap: cannot unwrap null"));
  }

  data.memory.machine_stack.emplace(wrapped);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetVTable(PassedExecutionData& data, const std::string& class_name) {
  auto vtable_idx = data.virtual_table_repository.GetIndexByName(class_name);

  if (!vtable_idx) {
    return std::unexpected(vtable_idx.error());
  }

  data.memory.machine_stack.emplace(static_cast<int64_t>(vtable_idx.value()));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetVTable(PassedExecutionData& data, const std::string& class_name) {
  auto argument = TryExtractArgument<void*>(data, "SetVTable");

  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto object_descriptor_ptr = reinterpret_cast<runtime::ObjectDescriptor*>(argument.value());

  auto vtable_idx = data.virtual_table_repository.GetIndexByName(class_name);

  if (!vtable_idx) {
    return std::unexpected(vtable_idx.error());
  }

  object_descriptor_ptr->vtable_index = static_cast<uint32_t>(vtable_idx.value());

  data.memory.machine_stack.emplace(reinterpret_cast<void*>(object_descriptor_ptr));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SafeCall(PassedExecutionData& data, const std::string& method) {
  auto nullable_obj = TryExtractArgument<void*>(data, "SafeCall");
  if (!nullable_obj) {
    return std::unexpected(nullable_obj.error());
  }

  void* nullable_obj_ptr = nullable_obj.value();
  auto* nullable_data_ptr = runtime::GetDataPointer<void*>(nullable_obj_ptr);

  if (*nullable_data_ptr == nullptr) {
    auto function = data.function_repository.GetByName(method);
    size_t method_arg_count = 0;

    if (function) {
      size_t arity = function.value()->GetArity();

      if (arity > 0) {
        method_arg_count = arity - 1; // -1 for this pointer
      }
    } else {
      // Count underscores in method name to get arity - no classname, virtual function
      auto underscores_count = std::count(method.begin(), method.end(), '_');
      method_arg_count = underscores_count - 2; // -1 for method name, -1 for this pointer
    }

    for (size_t i = 0; i < method_arg_count && !data.memory.machine_stack.empty(); ++i) {
      data.memory.machine_stack.pop();
    }

    data.memory.machine_stack.emplace(nullable_obj_ptr);

    return ExecutionResult::kNormal;
  }

  void* actual_obj = *nullable_data_ptr;
  auto function = data.function_repository.GetByName(method);
  std::expected<ExecutionResult, std::runtime_error> exec_result;

  if (function) {
    data.memory.machine_stack.emplace(actual_obj);
    exec_result = function.value()->Execute(data);
  } else {
    auto vtable =
        data.virtual_table_repository.GetByIndex(static_cast<runtime::ObjectDescriptor*>(actual_obj)->vtable_index);

    if (!vtable) {
      return std::unexpected(vtable.error());
    }

    auto function_id = vtable.value()->GetRealFunctionId(method);

    if (!function_id) {
      return std::unexpected(function_id.error());
    }

    const runtime::FunctionId const_function_id = std::move(function_id.value());
    auto vtable_function = data.function_repository.GetById(const_function_id);

    if (!vtable_function) {
      return std::unexpected(vtable_function.error());
    }

    data.memory.machine_stack.emplace(actual_obj);
    exec_result = vtable_function.value()->Execute(data);
  }

  if (!exec_result) {
    return std::unexpected(exec_result.error());
  }

  if (data.memory.machine_stack.empty()) {
    return ExecutionResult::kNormal;
  }

  runtime::Variable return_value = data.memory.machine_stack.top();

  if (std::holds_alternative<void*>(return_value)) {
    data.memory.machine_stack.pop();
    void* result_obj = std::get<void*>(return_value);
    auto push_nullable_result = PushNull(data);

    if (!push_nullable_result) {
      return std::unexpected(push_nullable_result.error());
    }

    runtime::Variable nullable_result_obj = data.memory.machine_stack.top();

    if (!std::holds_alternative<void*>(nullable_result_obj)) {
      return std::unexpected(std::runtime_error("SafeCall: nullable result object has incorrect type"));
    }

    void* nullable_result = std::get<void*>(nullable_result_obj);
    auto* nullable_result_data = runtime::GetDataPointer<void*>(nullable_result);
    *nullable_result_data = result_obj;

    return ExecutionResult::kNormal;
  }

  std::string constructor_name;
  runtime::Variable fundamental_value = return_value;

  if (std::holds_alternative<int64_t>(fundamental_value)) {
    constructor_name = "_Int_int";
  } else if (std::holds_alternative<double>(fundamental_value)) {
    constructor_name = "_Float_float";
  } else if (std::holds_alternative<bool>(fundamental_value)) {
    constructor_name = "_Bool_bool";
  } else if (std::holds_alternative<char>(fundamental_value)) {
    constructor_name = "_Char_char";
  } else if (std::holds_alternative<uint8_t>(fundamental_value)) {
    constructor_name = "_Byte_byte";
  } else {
    return std::unexpected(std::runtime_error("SafeCall: unknown return type"));
  }

  auto constructor_result = CallConstructor(data, constructor_name);

  if (!constructor_result) {
    return std::unexpected(constructor_result.error());
  }

  auto constructed_obj = TryExtractArgument<void*>(data, "SafeCall");

  if (!constructed_obj) {
    return std::unexpected(std::runtime_error("SafeCall: failed to extract constructed object"));
  }

  auto push_nullable_result = PushNull(data);

  if (!push_nullable_result) {
    return std::unexpected(push_nullable_result.error());
  }

  runtime::Variable nullable_result_obj = data.memory.machine_stack.top();

  if (!std::holds_alternative<void*>(nullable_result_obj)) {
    return std::unexpected(std::runtime_error("SafeCall: nullable result object has incorrect type"));
  }

  void* nullable_result = std::get<void*>(nullable_result_obj);
  auto* nullable_result_data = runtime::GetDataPointer<void*>(nullable_result);
  *nullable_result_data = constructed_obj.value();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NullCoalesce(PassedExecutionData& data) {
  auto tested_result = TryExtractArgument<void*>(data, "NullCoalesce");

  if (!tested_result) {
    return std::unexpected(tested_result.error());
  }

  auto* tested_result_data = runtime::GetDataPointer<void*>(tested_result.value());

  if (*tested_result_data != nullptr) {
    data.memory.machine_stack.pop();
    data.memory.machine_stack.emplace(tested_result.value());
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IsNull(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "IsNull");

  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* nullable_obj1 = argument.value();
  auto* nullable_ptr = runtime::GetDataPointer<void*>(nullable_obj1);

  data.memory.machine_stack.emplace(*nullable_ptr == nullptr);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Print(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "Print");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  data.output_stream << *str_ptr;

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PrintLine(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "PrintLine");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  data.output_stream << *str_ptr << '\n';

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ReadLine(PassedExecutionData& data) {
  std::string res;

  std::getline(data.input_stream, res);

  return PushString(data, res);
}

std::expected<ExecutionResult, std::runtime_error> ReadChar(PassedExecutionData& data) {
  char c = '\0';

  data.input_stream >> c;

  return PushChar(data, c);
}

std::expected<ExecutionResult, std::runtime_error> ReadInt(PassedExecutionData& data) {
  int64_t i = 0;

  data.input_stream >> i;

  return PushInt(data, i);
}

std::expected<ExecutionResult, std::runtime_error> ReadFloat(PassedExecutionData& data) {
  double d = 0.0;

  data.input_stream >> d;

  return PushFloat(data, d);
}

std::expected<ExecutionResult, std::runtime_error> UnixTime(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

  data.memory.machine_stack.emplace(static_cast<int64_t>(timestamp));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> UnixTimeMs(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

  data.memory.machine_stack.emplace(static_cast<int64_t>(timestamp));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> UnixTimeNs(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

  data.memory.machine_stack.emplace(static_cast<int64_t>(timestamp));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NanoTime(PassedExecutionData& data) {
  auto now = std::chrono::steady_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

  data.memory.machine_stack.emplace(static_cast<int64_t>(timestamp));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FormatDateTime(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, int64_t>(data, "FormatDateTime");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto timestamp_var = arguments.value().second;

  void* string_obj1 = arguments.value().first;
  auto* format_str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  try {
    auto time_point = std::chrono::system_clock::from_time_t(timestamp_var);
    auto time_t_val = std::chrono::system_clock::to_time_t(time_point);

    std::tm tm = *std::localtime(&time_t_val);

    std::stringstream ss;
    ss << std::put_time(&tm, format_str_ptr->c_str());
    std::string result_str = ss.str();

    auto vtable_result = data.virtual_table_repository.GetByName("String");
    if (!vtable_result.has_value()) {
      return std::unexpected(std::runtime_error("FormatDateTime: String vtable not found"));
    }

    const runtime::VirtualTable* string_vtable = vtable_result.value();
    auto vtable_index_result = data.virtual_table_repository.GetIndexByName("String");
    if (!vtable_index_result.has_value()) {
      return std::unexpected(vtable_index_result.error());
    }

    auto string_obj_result =
        data.memory_manager.AllocateObject(*string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data);

    if (!string_obj_result.has_value()) {
      return std::unexpected(string_obj_result.error());
    }

    void* string_obj = string_obj_result.value();
    auto* string_data = runtime::GetDataPointer<std::string>(string_obj);

    new (string_data) std::string(std::move(result_str));

    data.memory.machine_stack.emplace(string_obj);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("FormatDateTime: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> ParseDateTime(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "ParseDateTime");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto format_var = arguments.value().first;
  auto date_str_var = arguments.value().second;

  auto format_str_ptr = runtime::GetDataPointer<std::string>(format_var);
  auto date_str_ptr = runtime::GetDataPointer<std::string>(date_str_var);

  try {
    std::tm tm = {};
    std::stringstream ss(date_str_ptr->c_str());
    ss >> std::get_time(&tm, format_str_ptr->c_str());

    if (ss.fail()) {
      return std::unexpected(std::runtime_error("ParseDateTime: failed to parse date string"));
    }

    auto time_t_val = std::mktime(&tm);
    auto time_point = std::chrono::system_clock::from_time_t(time_t_val);
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(time_point.time_since_epoch()).count();

    auto vtable_result = data.virtual_table_repository.GetByName("Int");
    if (!vtable_result.has_value()) {
      return std::unexpected(std::runtime_error("ParseDateTime: Int vtable not found"));
    }

    const runtime::VirtualTable* int_vtable = vtable_result.value();
    auto vtable_index_result = data.virtual_table_repository.GetIndexByName("Int");
    if (!vtable_index_result.has_value()) {
      return std::unexpected(vtable_index_result.error());
    }

    auto int_obj_result =
        data.memory_manager.AllocateObject(*int_vtable, static_cast<uint32_t>(vtable_index_result.value()), data);

    if (!int_obj_result.has_value()) {
      return std::unexpected(int_obj_result.error());
    }

    void* int_obj = int_obj_result.value();
    auto* int_data = runtime::GetDataPointer<int64_t>(int_obj);
    *int_data = static_cast<int64_t>(timestamp);

    data.memory.machine_stack.emplace(int_obj);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("ParseDateTime: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> FileExists(PassedExecutionData& data) {
  auto filename_ptr = TryExtractArgument<void*>(data, "FileExists");
  if (!filename_ptr) {
    return std::unexpected(filename_ptr.error());
  }

  auto filename = runtime::GetDataPointer<std::string>(filename_ptr.value());

  try {
    bool exists = std::filesystem::exists(*filename);
    data.memory.machine_stack.emplace(exists);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("FileExists: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> DirectoryExists(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "DirectoryExists");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());

  try {
    bool exists = std::filesystem::is_directory(*dirname);
    data.memory.machine_stack.emplace(exists);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DirectoryExists: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> CreateDir(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "CreateDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());

  try {
    bool created = std::filesystem::create_directory(*dirname);
    data.memory.machine_stack.emplace(created);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("CreateDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> DeleteFileByName(PassedExecutionData& data) {
  auto filename_ptr = TryExtractArgument<void*>(data, "DeleteFile");
  if (!filename_ptr) {
    return std::unexpected(filename_ptr.error());
  }

  auto filename = runtime::GetDataPointer<std::string>(filename_ptr.value());

  try {
    bool deleted = std::filesystem::remove(*filename);
    data.memory.machine_stack.emplace(deleted);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DeleteFile: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> DeleteDir(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "DeleteDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());

  try {
    bool deleted = std::filesystem::remove_all(*dirname) > 0;
    data.memory.machine_stack.emplace(deleted);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DeleteDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> MoveFileByName(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "MoveFile");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto src_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto dest_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

  try {
    std::filesystem::rename(*src_ptr, *dest_ptr);
    data.memory.machine_stack.emplace(true);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> CopyFileByName(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "CopyFile");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto src_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto dest_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

  try {
    std::filesystem::copy(*src_ptr, *dest_ptr);
    data.memory.machine_stack.emplace(true);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> ListDir(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "ListDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());

  try {
    auto vtable_result = data.virtual_table_repository.GetByName("StringArray");
    if (!vtable_result.has_value()) {
      return std::unexpected(std::runtime_error("ListDirectory: StringArray vtable not found"));
    }

    const runtime::VirtualTable* string_array_vtable = vtable_result.value();
    auto vtable_index_result = data.virtual_table_repository.GetIndexByName("StringArray");
    if (!vtable_index_result.has_value()) {
      return std::unexpected(vtable_index_result.error());
    }

    auto string_array_obj_result = data.memory_manager.AllocateObject(
        *string_array_vtable, static_cast<uint32_t>(vtable_index_result.value()), data);
    if (!string_array_obj_result.has_value()) {
      return std::unexpected(string_array_obj_result.error());
    }

    void* string_array_obj = string_array_obj_result.value();
    auto* vec_data = runtime::GetDataPointer<std::vector<void*>>(string_array_obj);
    new (vec_data) std::vector<void*>();

    for (const auto& entry : std::filesystem::directory_iterator(*dirname)) {
      auto path_str = entry.path().string();

      auto string_vtable_result = data.virtual_table_repository.GetByName("String");
      if (!string_vtable_result.has_value()) {
        return std::unexpected(std::runtime_error("ListDirectory: String vtable not found"));
      }

      const runtime::VirtualTable* string_vtable = string_vtable_result.value();
      auto string_vtable_index_result = data.virtual_table_repository.GetIndexByName("String");
      if (!string_vtable_index_result.has_value()) {
        return std::unexpected(string_vtable_index_result.error());
      }

      auto string_obj_result = data.memory_manager.AllocateObject(
          *string_vtable, static_cast<uint32_t>(string_vtable_index_result.value()), data);

      if (!string_obj_result.has_value()) {
        return std::unexpected(string_obj_result.error());
      }

      void* string_obj = string_obj_result.value();
      auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
      new (string_data) std::string(path_str);

      vec_data->push_back(string_obj);
    }

    data.memory.machine_stack.emplace(string_array_obj);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("ListDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> GetCurrentDir(PassedExecutionData& data) {
  try {
    auto current_dir = std::filesystem::current_path().string();
    return PushString(data, current_dir);
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("GetCurrentDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> ChangeDir(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "ChangeDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());

  try {
    std::filesystem::current_path(*dirname);
    data.memory.machine_stack.emplace(true);
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.emplace(false);
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> SleepMs(PassedExecutionData& data) {
  auto ms_arg = TryExtractArgument<int64_t>(data, "SleepMs");
  if (!ms_arg) {
    return std::unexpected(ms_arg.error());
  }

  auto ms = ms_arg.value();
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SleepNs(PassedExecutionData& data) {
  auto ns_arg = TryExtractArgument<int64_t>(data, "SleepNs");
  if (!ns_arg) {
    return std::unexpected(ns_arg.error());
  }

  auto ns = ns_arg.value();
  std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Exit(PassedExecutionData& data) {
  auto exit_code_arg = TryExtractArgument<int64_t>(data, "Exit");
  if (!exit_code_arg) {
    return std::unexpected(exit_code_arg.error());
  }

  auto exit_code = exit_code_arg.value();
  std::exit(static_cast<int>(exit_code));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetProcessId(PassedExecutionData& data) {
#ifdef _WIN32
  auto pid = static_cast<int64_t>(GetCurrentProcessId());
#else
  auto pid = static_cast<int64_t>(getpid());
#endif
  data.memory.machine_stack.emplace(pid);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetEnvironmentVar(PassedExecutionData& data) {
  auto name_ptr = TryExtractArgument<void*>(data, "GetEnvironmentVariable");
  if (!name_ptr) {
    return std::unexpected(name_ptr.error());
  }

  auto name = runtime::GetDataPointer<std::string>(name_ptr.value());

  const char* value = std::getenv(name->c_str()); // NOLINT
  if (!value) {
    return PushNull(data);
  } else {
    auto null_result = PushNull(data);
    if (!null_result) {
      return std::unexpected(null_result.error());
    }

    auto string_result = PushString(data, std::string(value));
    if (!string_result) {
      return std::unexpected(string_result.error());
    }

    auto string_ptr = TryExtractArgument<void*>(data, "GetEnvironmentVariable");
    if (!string_ptr) {
      return std::unexpected(string_ptr.error());
    }

    runtime::Variable nullable_obj = data.memory.machine_stack.top();
    if (!std::holds_alternative<void*>(nullable_obj)) {
      return std::unexpected(
          std::runtime_error("GetEnvironmentVariable: variable on the top of the stack has incorrect type"));
    }

    void* nullable_ptr = std::get<void*>(nullable_obj);
    auto* nullable_value_ptr = runtime::GetDataPointer<void*>(nullable_ptr);
    *nullable_value_ptr = string_ptr.value();

    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> SetEnvironmentVar(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "SetEnvironmentVariable");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto name_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto value_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

#ifdef _WIN32
  bool success = SetEnvironmentVariable(name_ptr->c_str(), value_ptr->c_str()) != 0;
#else
  bool success = setenv(name_ptr->c_str(), value_ptr->c_str(), 1) == 0;
#endif

  data.memory.machine_stack.emplace(success);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Random(PassedExecutionData& data) {
  auto value = runtime_random_engine();
  data.memory.machine_stack.emplace(static_cast<int64_t>(value));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomRange(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "RandomRange");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto min = arguments.value().first;
  auto max = arguments.value().second;

  if (min > max) {
    std::swap(min, max);
  }

  std::uniform_int_distribution<int64_t> distribution(min, max);
  auto value = distribution(runtime_random_engine);

  data.memory.machine_stack.emplace(value);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomFloat(PassedExecutionData& data) {
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  auto value = distribution(runtime_random_engine);
  data.memory.machine_stack.emplace(value);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomFloatRange(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "RandomFloatRange");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto min = arguments.value().first;
  auto max = arguments.value().second;

  if (min > max) {
    std::swap(min, max);
  }

  std::uniform_real_distribution<double> distribution(min, max);
  auto value = distribution(runtime_random_engine);

  data.memory.machine_stack.emplace(value);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SeedRandom(PassedExecutionData& data) {
  auto seed_arg = TryExtractArgument<int64_t>(data, "SeedRandom");
  if (!seed_arg) {
    return std::unexpected(seed_arg.error());
  }

  auto seed = seed_arg.value();
  runtime_random_engine.seed(static_cast<uint64_t>(seed));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetMemoryUsage(PassedExecutionData& data) {
  size_t memory_usage = 0;

#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    memory_usage = static_cast<size_t>(pmc.WorkingSetSize);
  } else {
    return std::unexpected(std::runtime_error("GetMemoryUsage: failed to get process memory info"));
  }
#elif __APPLE__
  struct task_basic_info info;
  mach_msg_type_number_t size = sizeof(info);
  kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &size);
  if (kerr == KERN_SUCCESS) {
    memory_usage = static_cast<size_t>(info.resident_size);
  } else {
    return std::unexpected(std::runtime_error("GetMemoryUsage: failed to get task memory info"));
  }
#else
  // Linux: Use getrusage() system call
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    return std::unexpected(std::runtime_error("GetMemoryUsage: getrusage() failed"));
  }

  // ru_maxrss is the maximum resident set size in kilobytes
  // Note: getrusage gives maximum usage, not current usage
  memory_usage = static_cast<size_t>(usage.ru_maxrss) * 1024; // Convert KB to bytes
#endif

  data.memory.machine_stack.emplace(static_cast<int64_t>(memory_usage));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetPeakMemoryUsage(PassedExecutionData& data) {
  // For now, return same as current memory usage
  return GetMemoryUsage(data);
}

std::expected<ExecutionResult, std::runtime_error> ForceGarbageCollection(PassedExecutionData& data) {
  // Simple garbage collection: remove unreachable objects
  auto result = data.memory_manager.CollectGarbage(data);
  if (!result.has_value()) {
    return std::unexpected(result.error());
  }
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetProcessorCount(PassedExecutionData& data) {
  auto count = static_cast<int64_t>(std::thread::hardware_concurrency());
  data.memory.machine_stack.emplace(count);
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetOsName(PassedExecutionData& data) {
#ifdef _WIN32
  return PushString(data, "Windows");
#elif __APPLE__
  return PushString(data, "macOS");
#elif __linux__
  return PushString(data, "Linux");
#else
  return PushString(data, "Unknown");
#endif
}

std::expected<ExecutionResult, std::runtime_error> GetOsVersion(PassedExecutionData& data) {
  // Simple version string
#ifdef _WIN32
  return PushString(data, "Windows NT");
#elif __APPLE__
  return PushString(data, "Darwin");
#elif __linux__
  return PushString(data, "Linux");
#else
  return PushString(data, "Unknown");
#endif
}

std::expected<ExecutionResult, std::runtime_error> GetArchitecture(PassedExecutionData& data) {
#ifdef _WIN64
  return PushString(data, "x86_64");
#elif _WIN32
  return PushString(data, "x86");
#elif __x86_64__
  return PushString(data, "x86_64");
#elif __i386__
  return PushString(data, "x86");
#elif __aarch64__
  return PushString(data, "ARM64");
#elif __arm__
  return PushString(data, "ARM");
#else
  return PushString(data, "Unknown");
#endif
}

std::expected<ExecutionResult, std::runtime_error> GetUsername(PassedExecutionData& data) {
  const char* username = std::getenv("USERNAME"); // NOLINT
  if (!username) {
    username = std::getenv("USER"); // NOLINT
  }
  if (!username) {
    username = "Unknown";
  }
  return PushString(data, std::string(username));
}

std::expected<ExecutionResult, std::runtime_error> GetHomeDir(PassedExecutionData& data) {
  const char* homedir = std::getenv("HOME"); // NOLINT
  if (!homedir) {
    homedir = std::getenv("USERPROFILE"); // NOLINT
  }
  if (!homedir) {
    homedir = ".";
  }
  return PushString(data, std::string(homedir));
}

std::expected<ExecutionResult, std::runtime_error> TypeOf(PassedExecutionData& data) {
  runtime::Variable var = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();
  std::string type_name;
  if (std::holds_alternative<int64_t>(var)) {
    type_name = "int";
  } else if (std::holds_alternative<double>(var)) {
    type_name = "float";
  } else if (std::holds_alternative<bool>(var)) {
    type_name = "bool";
  } else if (std::holds_alternative<char>(var)) {
    type_name = "char";
  } else if (std::holds_alternative<uint8_t>(var)) {
    type_name = "byte";
  } else if (std::holds_alternative<void*>(var)) {
    if (std::get<void*>(var) == nullptr) {
      type_name = "Null";
    } else {
      auto vtable = data.virtual_table_repository.GetByIndex(
          static_cast<ovum::vm::runtime::ObjectDescriptor*>(std::get<void*>(var))->vtable_index);

      if (!vtable) {
        return std::unexpected(vtable.error());
      }

      type_name = vtable.value()->GetName();
    }
  }

  return PushString(data, type_name);
}

std::expected<ExecutionResult, std::runtime_error> IsType(PassedExecutionData& data, const std::string& type) {
  bool is_type = false;
  runtime::Variable var = data.memory.machine_stack.top();
  data.memory.machine_stack.pop();

  if (std::holds_alternative<int64_t>(var)) {
    is_type = type == "int";
  } else if (std::holds_alternative<double>(var)) {
    is_type = type == "float";
  } else if (std::holds_alternative<bool>(var)) {
    is_type = type == "bool";
  } else if (std::holds_alternative<char>(var)) {
    is_type = type == "char";
  } else if (std::holds_alternative<uint8_t>(var)) {
    is_type = type == "byte";
  } else if (std::holds_alternative<void*>(var)) {
    auto vtable = data.virtual_table_repository.GetByIndex(
        static_cast<ovum::vm::runtime::ObjectDescriptor*>(std::get<void*>(var))->vtable_index);

    if (!vtable) {
      return std::unexpected(vtable.error());
    }

    if (vtable.value()->GetName() != "Nullable") {
      is_type = vtable.value()->GetName() == type;
    } else {
      void* wrapped_var_ptr = std::get<void*>(var);
      auto* wrapped_var_data_ptr = runtime::GetDataPointer<void*>(wrapped_var_ptr);

      if (*wrapped_var_data_ptr == nullptr) {
        is_type = type == "Null";
      } else {
        auto wrapped_var_vtable = data.virtual_table_repository.GetByIndex(
            static_cast<ovum::vm::runtime::ObjectDescriptor*>(*wrapped_var_data_ptr)->vtable_index);

        if (!wrapped_var_vtable) {
          return std::unexpected(wrapped_var_vtable.error());
        }

        is_type = wrapped_var_vtable.value()->GetName() == type;
      }
    }
  }

  data.memory.machine_stack.emplace(is_type);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SizeOf(PassedExecutionData& data, const std::string& type) {
  size_t size = 0;

  if (type == "int") {
    size = sizeof(int64_t);
  } else if (type == "float") {
    size = sizeof(double);
  } else if (type == "bool") {
    size = sizeof(bool);
  } else if (type == "byte") {
    size = sizeof(uint8_t);
  } else if (type == "char") {
    size = sizeof(char);
  } else {
    auto vtable = data.virtual_table_repository.GetByName(type);

    if (!vtable) {
      return std::unexpected(vtable.error());
    }

    size = vtable.value()->GetSize();
  }

  data.memory.machine_stack.emplace(static_cast<int64_t>(size));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Interop(PassedExecutionData& data) {
  auto library_name_arg = TryExtractArgument<void*>(data, "Interop");
  if (!library_name_arg) {
    return std::unexpected(library_name_arg.error());
  }

  auto function_name_arg = TryExtractArgument<void*>(data, "Interop");
  if (!function_name_arg) {
    data.memory.machine_stack.emplace(*library_name_arg);
    return std::unexpected(function_name_arg.error());
  }

  auto input_array_arg = TryExtractArgument<void*>(data, "Interop");
  if (!input_array_arg) {
    data.memory.machine_stack.emplace(*function_name_arg);
    data.memory.machine_stack.emplace(*library_name_arg);
    return std::unexpected(input_array_arg.error());
  }

  auto output_array_arg = TryExtractArgument<void*>(data, "Interop");
  if (!output_array_arg) {
    data.memory.machine_stack.emplace(*input_array_arg);
    data.memory.machine_stack.emplace(*function_name_arg);
    data.memory.machine_stack.emplace(*library_name_arg);
    return std::unexpected(output_array_arg.error());
  }

  void* library_name_obj = library_name_arg.value();
  auto* library_name_str = runtime::GetDataPointer<std::string>(library_name_obj);
  void* function_name_obj = function_name_arg.value();
  auto* function_name_str = runtime::GetDataPointer<std::string>(function_name_obj);

  void* input_array_obj = input_array_arg.value();
  auto* input_byte_array = runtime::GetDataPointer<runtime::ByteArray>(input_array_obj);
  void* output_array_obj = output_array_arg.value();
  auto* output_byte_array = runtime::GetDataPointer<runtime::ByteArray>(output_array_obj);

  using FunctionPtr = long long (*)(void*, unsigned long long, void*, unsigned long long);

#ifdef _WIN32
  HMODULE handle = LoadLibraryA(library_name_str->c_str());
  if (!handle) {
    return std::unexpected(std::runtime_error("Interop: failed to load library " + *library_name_str));
  }

  auto func = reinterpret_cast<FunctionPtr>(GetProcAddress(handle, function_name_str->c_str()));
  if (!func) {
    FreeLibrary(handle);
    return std::unexpected(std::runtime_error("Interop: failed to find function " + *function_name_str +
                                              " in library " + *library_name_str));
  }
#else
  void* handle = dlopen(library_name_str->c_str(), RTLD_NOW);
  if (!handle) {
    return std::unexpected(
        std::runtime_error("Interop: failed to load library " + *library_name_str + ": " + dlerror()));
  }

  auto func = reinterpret_cast<FunctionPtr>(dlsym(handle, function_name_str->c_str()));
  if (!func) {
    const char* error = dlerror();
    dlclose(handle);
    return std::unexpected(std::runtime_error("Interop: failed to find function " + *function_name_str +
                                              " in library " + *library_name_str + ": " +
                                              (error ? error : "unknown error")));
  }
#endif

  void* input_data = input_byte_array->Data();
  auto input_size = static_cast<unsigned long long>(input_byte_array->Size());
  void* output_data = output_byte_array->Data();
  auto output_size = static_cast<unsigned long long>(output_byte_array->Size());

  long long result = func(input_data, input_size, output_data, output_size);

#ifdef _WIN32
  FreeLibrary(handle);
#else
  dlclose(handle);
#endif

  data.memory.machine_stack.emplace(static_cast<int64_t>(result));

  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree::bytecode
