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

namespace ovum::vm::execution_tree {

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

std::expected<ExecutionResult, std::runtime_error> PushString(PassedExecutionData& data, const std::string& value);
std::expected<ExecutionResult, std::runtime_error> PushNull(PassedExecutionData& data);

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

std::expected<ExecutionResult, std::runtime_error> CallConstructor(PassedExecutionData& data, const std::string& constructor);
std::expected<ExecutionResult, std::runtime_error> Unwrap(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> GetVTable(PassedExecutionData& data, const std::string& class_name) {
  auto vtable_idx = data.virtual_table_repository.GetIndexByName(class_name);

  if (!vtable_idx) {
    return std::unexpected(vtable_idx.error());
  }

  data.memory.machine_stack.push(static_cast<int64_t>(vtable_idx.value()));

  return ExecutionResult::kNormal;
}

std::expected<ExecutionResult, std::runtime_error> SetVTable(PassedExecutionData& data, const std::string& class_name) {
  auto argument = TryExtractArgument<void*>(data, "GetField");

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

std::expected<ExecutionResult, std::runtime_error> FormatDateTime(PassedExecutionData& data);



} // namespace ovum::vm::execution_tree