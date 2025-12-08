#include "BytecodeCommands.hpp"
#include <cmath>
#include <chrono>
#include <ctime>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include "FunctionRepository.hpp"
#include "IFunctionExecutable.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/executor/BuiltinFunctions.hpp"

namespace ovum::vm::execution_tree::bytecode {

static std::mt19937_64 runtime_random_engine(std::random_device{}());
static std::mutex runtime_random_mutex;

template<typename ArgumentType>
std::expected<ArgumentType, std::runtime_error> TryExtractArgument(PassedExecutionData& data, std::string function_name) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error(function_name + ": not enought arguments on the stack"));
  }

  runtime::Variable var_argument = data.memory.machine_stack.top();
  if (!std::holds_alternative<ArgumentType>(var_argument)) {
    return std::unexpected(std::runtime_error(function_name + ": variable on the top of the stack has incorrect type"));
  }

  return std::get<ArgumentType>(var_argument);
}

template<typename ArgumentOneType, typename ArgumentTwoType>
std::expected<std::pair<ArgumentOneType, ArgumentTwoType>, std::runtime_error> TryExtractTwoArguments(PassedExecutionData& data, std::string function_name) {
  auto argument_one = TryExtractArgument<ArgumentOneType>(data, function_name);
  if (!argument_one) {
    return std::unexpected(argument_one.error());
  }

  auto argument_two = TryExtractArgument<ArgumentTwoType>(data, function_name);
  if (!argument_two) {
    data.memory.machine_stack.push(runtime::Variable(*argument_one));
    return std::unexpected(argument_two.error());
  }

  return std::pair<ArgumentOneType, ArgumentTwoType>(*argument_one, *argument_two);
}

std::expected<ExecutionResult, std::runtime_error> PushInt(PassedExecutionData& data, int64_t value) {
  data.memory.machine_stack.push(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushFloat(PassedExecutionData& data, double value) {
  data.memory.machine_stack.push(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushBool(PassedExecutionData& data, bool value) {
  data.memory.machine_stack.push(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushChar(PassedExecutionData& data, char value) {
  data.memory.machine_stack.push(value);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushByte(PassedExecutionData& data, uint8_t value) {
  data.memory.machine_stack.push(value);

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

  auto string_obj_result = runtime::AllocateObject(
      *string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);

  if (!string_obj_result.has_value()) {
    return std::unexpected(string_obj_result.error());
  }

  void* string_obj = string_obj_result.value();
  auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
  new (string_data) std::string(std::move(value)); // Move to avoid copying
  data.memory.machine_stack.emplace(string_obj);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PushNull(PassedExecutionData& data) {
  auto vtable_result = data.virtual_table_repository.GetByName("Nullable");

  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("PushNull: String vtable not found"));
  }

  const runtime::VirtualTable* null_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("Nullable");

  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto null_obj_result = runtime::AllocateObject(
      *null_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);

  if (!null_obj_result.has_value()) {
    return std::unexpected(null_obj_result.error());
  }

  void* null_obj = null_obj_result.value();
  auto* null_data = runtime::GetDataPointer<void*>(null_obj);
  new (null_data) void*(std::move(nullptr)); // Move to avoid copying
  data.memory.machine_stack.emplace(null_obj);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Pop(PassedExecutionData& data) {
  data.memory.machine_stack.pop();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Dup(PassedExecutionData& data) {
  data.memory.machine_stack.push(data.memory.machine_stack.top());

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Swap(PassedExecutionData& data) {
  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("Swap: not enought arguments on the stack"));
  }

  runtime::Variable var_argument1 = data.memory.machine_stack.top();

  if (data.memory.machine_stack.empty()) {
    data.memory.machine_stack.push(var_argument1);
    return std::unexpected(std::runtime_error("Swap: not enought arguments on the stack"));
  }

  runtime::Variable var_argument2 = data.memory.machine_stack.top();

  data.memory.machine_stack.push(var_argument1);
  data.memory.machine_stack.push(var_argument2);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Rotate(PassedExecutionData& data, uint64_t n) {
  try {
    auto top = data.memory.machine_stack.top();
    data.memory.machine_stack.pop();

    std::vector<runtime::Variable> temp;
    temp.reserve(n - 1);

    for (int i = 0; i < n - 1; ++i) {
      temp.push_back(data.memory.machine_stack.top());
      data.memory.machine_stack.pop();
    }

    for (auto it = temp.rbegin(); it != temp.rend(); ++it) {
      data.memory.machine_stack.push(*it);
    }
    
    data.memory.machine_stack.push(top);

  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error("Rotate: not enough elements in stack"));
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> LoadLocal(PassedExecutionData& data, size_t index) {
  data.memory.machine_stack.push(data.memory.stack_frames.top().local_variables[index]);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetLocal(PassedExecutionData& data, size_t index) {
  data.memory.stack_frames.top().local_variables[index] = data.memory.machine_stack.top();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> LoadStatic(PassedExecutionData& data, size_t index) {
  data.memory.machine_stack.push(data.memory.global_variables[index]);

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetStatic(PassedExecutionData& data, size_t index) {
  data.memory.global_variables[index] = data.memory.machine_stack.top();

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ArrayGet(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ArraySet(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first + arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first - arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first * arguments->second));

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

  data.memory.machine_stack.push(runtime::Variable(arguments->first / arguments->second));

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

  data.memory.machine_stack.push(runtime::Variable(arguments->first % arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(-(*argument)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntIncrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntIncrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(*argument + 1));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntDecrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntDecrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(*argument - 1));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first + arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first - arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first * arguments->second));

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

  data.memory.machine_stack.push(runtime::Variable(arguments->first / arguments->second));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(-(*argument)));

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

  data.memory.machine_stack.push(runtime::Variable(std::sqrt(*argument)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteAdd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteAdd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first + arguments->second)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteSubtract(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteSubtract");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first - arguments->second)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteMultiply(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteMultiply");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first * arguments->second)));

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

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first / arguments->second)));

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

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first % arguments->second)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNegate(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteNegate");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(-(*argument))));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteIncrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteIncrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(*argument + 1)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteDecrement(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteDecrement");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(*argument - 1)));

  return ExecutionResult::kNormal;
}
std::expected<ExecutionResult, std::runtime_error> IntEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first == arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first != arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first < arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first <= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first > arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first >= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first == arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first != arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first < arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first <= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first > arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<double, double>(data, "FloatGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first >= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first == arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNotEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteNotEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first != arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLessThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLessThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first < arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLessEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLessEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first <= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteGreaterThan(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteGreaterThan");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first > arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteGreaterEqual(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteGreaterEqual");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first >= arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first && arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first || arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<bool>(data, "BoolNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(!(*argument)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<bool, bool>(data, "BoolXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first != arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first & arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first | arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first ^ arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<int64_t>(data, "IntNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(~(*argument)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntLeftShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntLeftShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first << arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> IntRightShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "IntRightShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(arguments->first >> arguments->second));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteAnd(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteAnd");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first & arguments->second)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteOr(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteOr");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first | arguments->second)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteXor(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteXor");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first ^ arguments->second)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteNot(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteNot");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(~(*argument))));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteLeftShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteLeftShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first << arguments->second)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteRightShift(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<uint8_t, uint8_t>(data, "ByteRightShift");
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(arguments->first >> arguments->second)));
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

  auto res_ptr = new std::string;

  res_ptr->append(*str1_ptr);
  res_ptr->append(*str2_ptr);

  return PushString(data, *res_ptr);
}

std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringLength");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(str_ptr->length())));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringSubstring(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringSubstring");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "StringSubstring");
  if (!arguments) {
    data.memory.machine_stack.push(runtime::Variable(argument.value()));
    return std::unexpected(arguments.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);
  auto res_ptr = new std::string;
  res_ptr->append(str_ptr->substr(arguments.value().first, arguments.value().second));

  return PushString(data, *res_ptr);
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

  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(res)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> StringToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  auto res = std::stol(*str_ptr);

  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(res)));

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

  data.memory.machine_stack.push(runtime::Variable(static_cast<double>(res)));

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

  data.memory.machine_stack.push(runtime::Variable(static_cast<double>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> FloatToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<double>(data, "FloatToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteToInt(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteToInt");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> CharToByte(PassedExecutionData& data) {
  auto argument = TryExtractArgument<char>(data, "CharToByte");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ByteToChar(PassedExecutionData& data) {
  auto argument = TryExtractArgument<uint8_t>(data, "ByteToChar");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<char>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> BoolToByte(PassedExecutionData& data) {
  auto argument = TryExtractArgument<bool>(data, "BoolToByte");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  data.memory.machine_stack.push(runtime::Variable(static_cast<uint8_t>(argument.value())));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Call(PassedExecutionData& data, const std::string& function_name) {
  auto function = data.function_repository.GetByName(function_name);

  if (!function) {
    return std::unexpected(function.error());
  }

  function.value()->Execute(data);

  return ExecutionResult::kNormal;
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

  auto vtable = data.virtual_table_repository.GetByIndex(static_cast<ovum::vm::runtime::ObjectDescriptor*>(argument.value())->vtable_index);

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

  return function.value()->Execute(data);
}

std::expected<ExecutionResult, std::runtime_error> Return(PassedExecutionData& data) {
  return ExecutionResult::kReturn;
}

std::expected<ExecutionResult, std::runtime_error> GetField(PassedExecutionData& data, size_t number) {
  auto argument = TryExtractArgument<void*>(data, "GetField");

  if (!argument) {
    return std::unexpected(argument.error());
  }

  auto vtable = data.virtual_table_repository.GetByIndex(reinterpret_cast<runtime::ObjectDescriptor*>(argument.value())->vtable_index);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto field = vtable.value()->GetVariableByIndex(argument.value(), number);

  if (!field) {
    return std::unexpected(field.error());
  }

  data.memory.machine_stack.push(field.value());

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetField(PassedExecutionData& data, size_t number) {
  auto argument1 = TryExtractArgument<void*>(data, "SetField");

  if (!argument1) {
    return std::unexpected(argument1.error());
  }

  if (data.memory.machine_stack.empty()) {
    return std::unexpected(std::runtime_error("SetField: not enought arguments on the stack"));
  }

  runtime::Variable argument2 = data.memory.machine_stack.top();

  auto vtable = data.virtual_table_repository.GetByIndex(reinterpret_cast<runtime::ObjectDescriptor*>(argument1.value())->vtable_index);

  if (!vtable) {
    return std::unexpected(vtable.error());
  }

  auto result = vtable.value()->SetVariableByIndex(argument1.value(), number, argument2);

  if (!result) {
    return std::unexpected(result.error());
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> CallConstructor(PassedExecutionData& data, const std::string& constructor) {

}

std::expected<ExecutionResult, std::runtime_error> Unwrap(PassedExecutionData& data) {
  auto res = Dup(data);
  if (!res) {
    return std::unexpected(res.error());
  }

  res = IsNull(data);
  if (!res) {
    data.memory.machine_stack.pop();
    return std::unexpected(res.error());
  }

  auto is_null = TryExtractArgument<bool>(data, "Unwrap");
  if (!is_null) {
    return std::unexpected(is_null.error());
  }

  if (is_null.value()) {
    return std::unexpected(std::runtime_error("Unwrap: cannot unwrap null"));
  } else {
    auto nullable_result = TryExtractArgument<void*>(data, "Unwrap");

    if (!nullable_result) {
      return std::unexpected(nullable_result.error());
    }

    auto result = runtime::GetDataPointer<void*>(nullable_result.value());

    data.memory.machine_stack.push(runtime::Variable(*result));
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetVTable(PassedExecutionData& data, const std::string& class_name) {
  auto vtable_idx = data.virtual_table_repository.GetIndexByName(class_name);

  if (!vtable_idx) {
    return std::unexpected(vtable_idx.error());
  }

  data.memory.machine_stack.push(static_cast<int64_t>(vtable_idx.value()));

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

  data.memory.machine_stack.push(reinterpret_cast<void*>(object_descriptor_ptr));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SafeCall(PassedExecutionData& data, const std::string& method) {
  auto argument = TryExtractArgument<void*>(data, "IsNull");

  if (!argument) {
    return std::unexpected(argument.error());
  }
  
  void* nullable_obj1 = argument.value();
  auto* nullable_ptr = runtime::GetDataPointer<void*>(nullable_obj1);

  if (*nullable_ptr != nullptr) {
    auto vtable = data.virtual_table_repository.GetByIndex(reinterpret_cast<runtime::ObjectDescriptor*>(nullable_ptr)->vtable_index);

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

    auto res_status = function.value()->Execute(data);

    if (!res_status) {
      return std::unexpected(res_status.error());
    }

    auto result = TryExtractArgument<void*>(data, "SafeCall");

    if (result) {
      auto r = PushNull(data);
      if (!r) {
        return std::unexpected(r.error());
      }

      auto* res_nullable_ptr = runtime::GetDataPointer<void*>(nullable_obj1);
      *res_nullable_ptr = result.value();
    } else {
      void* result_ptr = nullptr;

      std::string type_name;
      if (type_name == "") {
        auto arg = TryExtractArgument<int64_t>(data, "SafeCall");

        if (arg) {
          type_name = "Int";
          result_ptr = &arg.value();
        }
      }
    
      if (type_name == "") {
        auto arg = TryExtractArgument<double>(data, "SafeCall");

        if (arg) {
          type_name = "Double";
          result_ptr = &arg.value();
        }
      }

      if (type_name == "") {
        auto arg = TryExtractArgument<bool>(data, "SafeCall");

        if (arg) {
          type_name = "Bool";
          result_ptr = &arg.value();
        }
      }

      if (type_name == "") {
        auto arg = TryExtractArgument<char>(data, "SafeCall");

        if (arg) {
          type_name = "Char";
          result_ptr = &arg.value();
        }
      }

      if (type_name == "") {
        auto arg = TryExtractArgument<uint8_t>(data, "SafeCall");

        if (arg) {
          type_name = "Byte";
          result_ptr = &arg.value();
        }
      }

      if (type_name == "") {
        return std::unexpected(std::runtime_error("SafeCall: unknown type of return argument"));
      }
      auto vtable_result = data.virtual_table_repository.GetByName(type_name);
      if (!vtable_result.has_value()) {
        return std::unexpected(std::runtime_error("SafeCall: File vtable not found"));
      }
    
      const runtime::VirtualTable* vtable = vtable_result.value();
      auto vtable_index_result = data.virtual_table_repository.GetIndexByName(type_name);
      if (!vtable_index_result.has_value()) {
        return std::unexpected(vtable_index_result.error());
      }

      auto obj_result = runtime::AllocateObject(
        *vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);

      if (!obj_result) {
        return std::unexpected(obj_result.error());
      }
          
      runtime::StackFrame frame;
      frame.local_variables.resize(2);

      frame.local_variables[0] = obj_result.value();
      frame.local_variables[1] = runtime::Variable(data.memory.machine_stack.top());

      data.memory.stack_frames.push(std::move(frame));

      std::expected<ovum::vm::execution_tree::ExecutionResult, std::runtime_error> constructor_result;
      if (type_name == "Int") {
        constructor_result = execution_tree::IntConstructor(data);
      } else if (type_name == "Byte") {
        constructor_result = execution_tree::ByteConstructor(data);
      } else if (type_name == "Float") {
        constructor_result = execution_tree::FloatConstructor(data);
      } else if (type_name == "Bool") {
        constructor_result = execution_tree::BoolConstructor(data);
      } else if (type_name == "Char") {
        constructor_result = execution_tree::CharConstructor(data);
      } else {
        constructor_result = std::unexpected(std::runtime_error("SafeCall: unknown type of return value"));
      }

      data.memory.stack_frames.pop();

      if (!constructor_result) {
        return std::unexpected(constructor_result.error());
      }

      auto result2 = TryExtractArgument<void*>(data, "SafeCall");

      if (result2) {
        auto r = PushNull(data);
        if (!r) {
          return std::unexpected(r.error());
        }

        auto* res_nullable_ptr = runtime::GetDataPointer<void*>(nullable_obj1);
        *res_nullable_ptr = result2.value();
      } else {
        return std::unexpected(std::runtime_error("SafeCall: incorrect argument on the stack after convertation"));
      }
    }
  } else {
    // Do nothing
  }

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NullCoalesce(PassedExecutionData& data) {
  auto res = Dup(data);
  if (!res) {
    return std::unexpected(res.error());
  }

  res = IsNull(data);
  if (!res) {
    data.memory.machine_stack.pop();
    return std::unexpected(res.error());
  }

  auto is_null = TryExtractArgument<bool>(data, "NullCoalesce");
  if (!is_null) {
    return std::unexpected(is_null.error());
  }

  if (is_null.value()) {
    data.memory.machine_stack.pop();
  } else {
    auto result = data.memory.machine_stack.top();
    data.memory.machine_stack.pop();
    data.memory.machine_stack.pop();
    data.memory.machine_stack.push(result);
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

  data.memory.machine_stack.push(runtime::Variable(*nullable_ptr == nullptr));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> Print(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringLength");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  std::cout << *str_ptr;

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> PrintLine(PassedExecutionData& data) {
  auto argument = TryExtractArgument<void*>(data, "StringLength");
  if (!argument) {
    return std::unexpected(argument.error());
  }

  void* string_obj1 = argument.value();
  auto* str_ptr = runtime::GetDataPointer<std::string>(string_obj1);

  std::cout << *str_ptr << std::endl;

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> ReadLine(PassedExecutionData& data) {
  std::string res;

  std::getline(std::cin, res);

  return PushString(data, res);
}

std::expected<ExecutionResult, std::runtime_error> ReadChar(PassedExecutionData& data) {
  char c;

  std::cin >> c;

  return PushChar(data, c);
}

std::expected<ExecutionResult, std::runtime_error> ReadInt(PassedExecutionData& data) {
  int64_t i;

  std::cin >> i;

  return PushInt(data, i);
}

std::expected<ExecutionResult, std::runtime_error> ReadFloat(PassedExecutionData& data) {
  double d;

  std::cin >> d;

  return PushFloat(data, d);
}

std::expected<ExecutionResult, std::runtime_error> UnixTime(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
    now.time_since_epoch()).count();
    
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(timestamp)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> UnixTimeMs(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
    now.time_since_epoch()).count();
  
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(timestamp)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> UnixTimeNs(PassedExecutionData& data) {
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
      now.time_since_epoch()).count();
  
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(timestamp)));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> NanoTime(PassedExecutionData& data) {
  auto now = std::chrono::steady_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
    now.time_since_epoch()).count();
  
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(timestamp)));

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

    auto string_obj_result = runtime::AllocateObject(
      *string_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
    if (!string_obj_result.has_value()) {
      return std::unexpected(string_obj_result.error());
    }

    void* string_obj = string_obj_result.value();
    auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
    
    new (string_data) std::string(std::move(result_str));
    
    data.memory.machine_stack.push(runtime::Variable(string_obj));
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
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
      time_point.time_since_epoch()).count();
    
    auto vtable_result = data.virtual_table_repository.GetByName("Int");
    if (!vtable_result.has_value()) {
      return std::unexpected(std::runtime_error("ParseDateTime: Int vtable not found"));
    }

    const runtime::VirtualTable* int_vtable = vtable_result.value();
    auto vtable_index_result = data.virtual_table_repository.GetIndexByName("Int");
    if (!vtable_index_result.has_value()) {
      return std::unexpected(vtable_index_result.error());
    }

    auto int_obj_result = runtime::AllocateObject(
      *int_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
    if (!int_obj_result.has_value()) {
      return std::unexpected(int_obj_result.error());
    }

    void* int_obj = int_obj_result.value();
    auto* int_data = runtime::GetDataPointer<int64_t>(int_obj);
    *int_data = static_cast<int64_t>(timestamp);
    
    data.memory.machine_stack.push(runtime::Variable(int_obj));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("ParseDateTime: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> OpenFile(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "FormatDateTime");
  
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto mode_var = arguments.value().first;
  auto filename_var = arguments.value().second;

  auto vtable_result = data.virtual_table_repository.GetByName("File");
  if (!vtable_result.has_value()) {
    return std::unexpected(std::runtime_error("OpenFile: File vtable not found"));
  }

  const runtime::VirtualTable* file_vtable = vtable_result.value();
  auto vtable_index_result = data.virtual_table_repository.GetIndexByName("File");
  if (!vtable_index_result.has_value()) {
    return std::unexpected(vtable_index_result.error());
  }

  auto file_obj_result = runtime::AllocateObject(
    *file_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
  if (!file_obj_result.has_value()) {
    return std::unexpected(file_obj_result.error());
  }

  void* file_obj = file_obj_result.value();
  
  {
    runtime::StackFrame frame;
    frame.local_variables.resize(1);
    frame.local_variables[0] = file_obj;
    
    data.memory.stack_frames.push(std::move(frame));
    auto ctor_result = ovum::vm::execution_tree::FileConstructor(data);
    data.memory.stack_frames.pop();
    
    if (!ctor_result) {
      return std::unexpected(ctor_result.error());
    }
  }
  
  runtime::StackFrame frame;
  frame.local_variables.resize(3);
  
  frame.local_variables[0] = file_obj;
  frame.local_variables[1] = filename_var;
  frame.local_variables[2] = mode_var;
  
  data.memory.stack_frames.push(std::move(frame));
  
  auto result = ovum::vm::execution_tree::FileOpen(data);
  
  data.memory.stack_frames.pop();
  
  if (!result) {
    runtime::StackFrame dtor_frame;
    dtor_frame.local_variables.resize(1);
    dtor_frame.local_variables[0] = file_obj;
    
    data.memory.stack_frames.push(std::move(dtor_frame));
    ovum::vm::execution_tree::FileDestructor(data);
    data.memory.stack_frames.pop();
    
    return std::unexpected(result.error());
  }
  
  data.memory.machine_stack.push(runtime::Variable(file_obj));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> CloseFile(PassedExecutionData& data) {
  auto file_ptr = TryExtractArgument<void*>(data, "CloseFile");
  if (!file_ptr) {
    return std::unexpected(file_ptr.error());
  }

  runtime::StackFrame frame;
  frame.local_variables.resize(1);
  frame.local_variables[0] = file_ptr.value();
  
  data.memory.stack_frames.push(std::move(frame));
  
  auto result = ovum::vm::execution_tree::FileClose(data);
  
  data.memory.stack_frames.pop();
  
  return result;
}

std::expected<ExecutionResult, std::runtime_error> FileExists(PassedExecutionData& data) {
  auto filename_ptr = TryExtractArgument<void*>(data, "FileExists");
  if (!filename_ptr) {
    return std::unexpected(filename_ptr.error());
  }

  auto filename = runtime::GetDataPointer<std::string>(filename_ptr.value());
  
  try {
    bool exists = std::filesystem::exists(*filename);
    data.memory.machine_stack.push(runtime::Variable(exists));
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
    data.memory.machine_stack.push(runtime::Variable(exists));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DirectoryExists: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> CreateDirectory(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "CreateDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());
  
  try {
    bool created = std::filesystem::create_directory(*dirname);
    data.memory.machine_stack.push(runtime::Variable(created));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("CreateDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> DeleteFile(PassedExecutionData& data) {
  auto filename_ptr = TryExtractArgument<void*>(data, "DeleteFile");
  if (!filename_ptr) {
    return std::unexpected(filename_ptr.error());
  }

  auto filename = runtime::GetDataPointer<std::string>(filename_ptr.value());
  
  try {
    bool deleted = std::filesystem::remove(*filename);
    data.memory.machine_stack.push(runtime::Variable(deleted));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DeleteFile: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> DeleteDirectory(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "DeleteDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());
  
  try {
    bool deleted = std::filesystem::remove_all(*dirname) > 0;
    data.memory.machine_stack.push(runtime::Variable(deleted));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("DeleteDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> MoveFile(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "MoveFile");

  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto src_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto dest_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

  try {
    std::filesystem::rename(*src_ptr, *dest_ptr);
    data.memory.machine_stack.push(runtime::Variable(true));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.push(runtime::Variable(false));
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> CopyFile(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "MoveFile");
  
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto src_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto dest_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

  try {
    std::filesystem::copy(*src_ptr, *dest_ptr);
    data.memory.machine_stack.push(runtime::Variable(true));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.push(runtime::Variable(false));
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> ListDirectory(PassedExecutionData& data) {
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

    auto string_array_obj_result = runtime::AllocateObject(
        *string_array_vtable, static_cast<uint32_t>(vtable_index_result.value()), data.memory.object_repository);
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
      
      auto string_obj_result = runtime::AllocateObject(
          *string_vtable, static_cast<uint32_t>(string_vtable_index_result.value()), data.memory.object_repository);
      if (!string_obj_result.has_value()) {
        return std::unexpected(string_obj_result.error());
      }
      
      void* string_obj = string_obj_result.value();
      auto* string_data = runtime::GetDataPointer<std::string>(string_obj);
      new (string_data) std::string(path_str);
      
      vec_data->push_back(string_obj);
    }
    
    data.memory.machine_stack.push(runtime::Variable(string_array_obj));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("ListDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> GetCurrentDirectory(PassedExecutionData& data) {
  try {
    auto current_dir = std::filesystem::current_path().string();
    return PushString(data, current_dir);
  } catch (const std::exception& e) {
    return std::unexpected(std::runtime_error(std::string("GetCurrentDirectory: ") + e.what()));
  }
}

std::expected<ExecutionResult, std::runtime_error> ChangeDirectory(PassedExecutionData& data) {
  auto dirname_ptr = TryExtractArgument<void*>(data, "ChangeDirectory");
  if (!dirname_ptr) {
    return std::unexpected(dirname_ptr.error());
  }

  auto dirname = runtime::GetDataPointer<std::string>(dirname_ptr.value());
  
  try {
    std::filesystem::current_path(*dirname);
    data.memory.machine_stack.push(runtime::Variable(true));
    return ExecutionResult::kNormal;
  } catch (const std::exception& e) {
    data.memory.machine_stack.push(runtime::Variable(false));
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
  data.memory.machine_stack.push(runtime::Variable(pid));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetEnvironmentVariable(PassedExecutionData& data) {
  auto name_ptr = TryExtractArgument<void*>(data, "GetEnvironmentVariable");
  if (!name_ptr) {
    return std::unexpected(name_ptr.error());
  }

  auto name = runtime::GetDataPointer<std::string>(name_ptr.value());
  
  const char* value = std::getenv(name->c_str());
  if (value) {
    return PushString(data, std::string(value));
  } else {
    data.memory.machine_stack.push(runtime::Variable(static_cast<void*>(nullptr)));
    return ExecutionResult::kNormal;
  }
}

std::expected<ExecutionResult, std::runtime_error> SetEnvironmentVariable(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<void*, void*>(data, "MoveFile");
  
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto name_ptr = runtime::GetDataPointer<std::string>(arguments.value().first);
  auto value_ptr = runtime::GetDataPointer<std::string>(arguments.value().second);

#ifdef _WIN32
  bool success = SetEnvironmentVariableA(name_ptr->c_str(), value_ptr->c_str()) != 0;
#else
  bool success = setenv(name_ptr->c_str(), value_ptr->c_str(), 1) == 0;
#endif

  data.memory.machine_stack.push(runtime::Variable(success));
  return ExecutionResult::kNormal;
}


std::expected<ExecutionResult, std::runtime_error> Random(PassedExecutionData& data) {
  std::lock_guard<std::mutex> lock(runtime_random_mutex);
  auto value = runtime_random_engine();
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(value)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomRange(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "MoveFile");
  
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto min = arguments.value().first;
  auto max = arguments.value().second;

  if (min > max) {
    std::swap(min, max);
  }

  std::lock_guard<std::mutex> lock(runtime_random_mutex);
  std::uniform_int_distribution<int64_t> distribution(min, max);
  auto value = distribution(runtime_random_engine);
  
  data.memory.machine_stack.push(runtime::Variable(value));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomFloat(PassedExecutionData& data) {
  std::lock_guard<std::mutex> lock(runtime_random_mutex);
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  auto value = distribution(runtime_random_engine);
  data.memory.machine_stack.push(runtime::Variable(value));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> RandomFloatRange(PassedExecutionData& data) {
  auto arguments = TryExtractTwoArguments<int64_t, int64_t>(data, "MoveFile");
  
  if (!arguments) {
    return std::unexpected(arguments.error());
  }

  auto min = arguments.value().first;
  auto max = arguments.value().second;

  if (min > max) {
    std::swap(min, max);
  }

  std::lock_guard<std::mutex> lock(runtime_random_mutex);
  std::uniform_real_distribution<double> distribution(min, max);
  auto value = distribution(runtime_random_engine);
  
  data.memory.machine_stack.push(runtime::Variable(value));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SeedRandom(PassedExecutionData& data) {
  auto seed_arg = TryExtractArgument<int64_t>(data, "SeedRandom");
  if (!seed_arg) {
    return std::unexpected(seed_arg.error());
  }

  auto seed = seed_arg.value();
  std::lock_guard<std::mutex> lock(runtime_random_mutex);
  runtime_random_engine.seed(static_cast<uint64_t>(seed));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetMemoryUsage(PassedExecutionData& data) {
  // Simple memory usage approximation
  size_t memory_usage = 0;
  
  // Stack size
  memory_usage += data.memory.machine_stack.size() * sizeof(runtime::Variable);
  
  auto stack_copy = data.memory.stack_frames;
  // Local variables in all stack frames
  for (; ; stack_copy.pop()) {
    memory_usage += data.memory.stack_frames.top().local_variables.size() * sizeof(runtime::Variable);
  }
  
  // TODO counter for repositoties
  
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(memory_usage)));
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetPeakMemoryUsage(PassedExecutionData& data) {
  // For now, return same as current memory usage
  return GetMemoryUsage(data);
}

std::expected<ExecutionResult, std::runtime_error> ForceGarbageCollection(PassedExecutionData& data) {
  // Simple garbage collection: remove unreachable objects
  // This is a placeholder implementation
  //data.memory.object_repository.CollectGarbage();
  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> GetProcessorCount(PassedExecutionData& data) {
  auto count = static_cast<int64_t>(std::thread::hardware_concurrency());
  data.memory.machine_stack.push(runtime::Variable(count));
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

std::expected<ExecutionResult, std::runtime_error> GetUserName(PassedExecutionData& data) {
  const char* username = std::getenv("USERNAME");
  if (!username) {
    username = std::getenv("USER");
  }
  if (!username) {
    username = "Unknown";
  }
  return PushString(data, std::string(username));
}

std::expected<ExecutionResult, std::runtime_error> GetHomeDirectory(PassedExecutionData& data) {
  const char* homedir = std::getenv("HOME");
  if (!homedir) {
    homedir = std::getenv("USERPROFILE");
  }
  if (!homedir) {
    homedir = ".";
  }
  return PushString(data, std::string(homedir));
}

std::expected<ExecutionResult, std::runtime_error> TypeOf(PassedExecutionData& data) {
  std::string type_name;
  if (type_name == "") {
    auto arg = TryExtractArgument<int64_t>(data, "TypeOf");
    
    if (arg) {
      type_name = "Int";
    }
  }

  if (type_name == "") {
    auto arg = TryExtractArgument<double>(data, "TypeOf");
    
    if (arg) {
      type_name = "Double";
    }
  }
  
  if (type_name == "") {
    auto arg = TryExtractArgument<bool>(data, "TypeOf");
    
    if (arg) {
      type_name = "Bool";
    }
  }
  
  if (type_name == "") {
    auto arg = TryExtractArgument<char>(data, "TypeOf");
    
    if (arg) {
      type_name = "Char";
    }
  }
  
  if (type_name == "") {
    auto arg = TryExtractArgument<uint8_t>(data, "TypeOf");
    
    if (arg) {
      type_name = "Byte";
    }
  }

  if (type_name == "") {
    auto arg = TryExtractArgument<void*>(data, "TypeOf");
    
    if (arg) {
      if (arg.value() == nullptr) {
        type_name = "Null";
      } else {
        
        auto vtable = data.virtual_table_repository.GetByIndex(
          static_cast<ovum::vm::runtime::ObjectDescriptor*>(arg.value())->vtable_index);
        
        if (!vtable) {
          return std::unexpected(vtable.error());
        }
      
        type_name = vtable.value()->GetName();
      }
    } else {
      return std::unexpected(arg.error());
    }
  }

  return PushString(data, type_name);
}

std::expected<ExecutionResult, std::runtime_error> IsType(PassedExecutionData& data, const std::string& type) {
  bool is_type = false;
  
  if (type == "Int") {
    is_type = TryExtractArgument<int64_t>(data, "IsType").has_value();
  } else if (type == "Float") {
    is_type = TryExtractArgument<double>(data, "IsType").has_value();
  } else if (type == "Bool") {
    is_type = TryExtractArgument<double>(data, "IsType").has_value();
  } else if (type == "Char") {
    is_type = TryExtractArgument<char>(data, "IsType").has_value();
  } else if (type == "Byte") {
    is_type = TryExtractArgument<uint8_t>(data, "IsType").has_value();
  } else if (type == "Null") {
    auto arg = TryExtractArgument<void*>(data, "IsType");
    if (arg) {
      is_type = arg.value() == nullptr;
    }
  } else {
    auto argument = TryExtractArgument<void*>(data, "IsType");

    if (!argument) {
      return std::unexpected(argument.error());
    }

    auto vtable = data.virtual_table_repository.GetByIndex(
      static_cast<ovum::vm::runtime::ObjectDescriptor*>(argument.value())->vtable_index);

    if (!vtable) {
      return std::unexpected(vtable.error());
    }

    is_type = vtable.value()->IsType(type);
  }
  
  data.memory.machine_stack.push(runtime::Variable(is_type));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SizeOf(PassedExecutionData& data, const std::string& type) {
  size_t size = 0;
  
  if (type == "Int") {
    size = sizeof(int64_t);
  } else if (type == "Float") {
    size = sizeof(double);
  } else if (type == "Bool") {
    size = sizeof(bool);
  } else if (type == "Byte") {
    size = sizeof(uint8_t);
  } else if (type == "Char") {
    size = sizeof(char);
  } else {
    auto vtable = data.virtual_table_repository.GetByName(type);

    if (!vtable) {
      return std::unexpected(vtable.error());
    }

    size = vtable.value()->GetSize();
  }
  
  data.memory.machine_stack.push(runtime::Variable(static_cast<int64_t>(size)));
  return ExecutionResult::kNormal;
}

} // namespace ovum::vm::execution_tree