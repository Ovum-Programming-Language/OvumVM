#include "BytecodeCommands.hpp"
#include <cmath>
#include <chrono>
#include <random>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <thread>
#include "FunctionRepository.hpp"
#include "IFunctionExecutable.hpp"
#include "../runtime/ObjectDescriptor.hpp"

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
} // namespace ovum::vm::execution_tree