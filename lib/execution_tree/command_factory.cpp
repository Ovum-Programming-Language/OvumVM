#include "command_factory.hpp"

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "BytecodeCommands.hpp"
#include "Command.hpp"

namespace ovum::vm::execution_tree {

namespace {

using SimpleCommandFunc = std::function<std::expected<ExecutionResult, std::runtime_error>(PassedExecutionData&)>;
using StringCommandFunc =
    std::function<std::expected<ExecutionResult, std::runtime_error>(PassedExecutionData&, const std::string&)>;
using IntegerCommandFunc =
    std::function<std::expected<ExecutionResult, std::runtime_error>(PassedExecutionData&, int64_t)>;
using FloatCommandFunc =
    std::function<std::expected<ExecutionResult, std::runtime_error>(PassedExecutionData&, double)>;
using BooleanCommandFunc =
    std::function<std::expected<ExecutionResult, std::runtime_error>(PassedExecutionData&, bool)>;

// Helper function to create a command with captured argument
template<typename Func, typename Arg>
std::unique_ptr<IExecutable> CreateCommandWithArg(Func func, Arg arg) {
  auto wrapped_func = [func, arg](PassedExecutionData& data) { return func(data, arg); };
  return std::make_unique<Command<decltype(wrapped_func)>>(std::move(wrapped_func));
}

// Hashmaps for command lookup
const std::unordered_map<std::string, SimpleCommandFunc>& GetSimpleCommands() {
  static const std::unordered_map<std::string, SimpleCommandFunc> kMap = {
      // Stack operations
      {"Pop", bytecode::Pop},
      {"Dup", bytecode::Dup},
      {"Swap", bytecode::Swap},
      {"PushNull", bytecode::PushNull},

      // Control flow
      {"Return", bytecode::Return},

      // Arithmetic operations (unary)
      {"IntNegate", bytecode::IntNegate},
      {"IntIncrement", bytecode::IntIncrement},
      {"IntDecrement", bytecode::IntDecrement},
      {"FloatNegate", bytecode::FloatNegate},
      {"FloatSqrt", bytecode::FloatSqrt},
      {"ByteNegate", bytecode::ByteNegate},
      {"ByteIncrement", bytecode::ByteIncrement},
      {"ByteDecrement", bytecode::ByteDecrement},

      // Logical operations (unary)
      {"BoolNot", bytecode::BoolNot},
      {"IntNot", bytecode::IntNot},
      {"ByteNot", bytecode::ByteNot},

      // Binary arithmetic operations
      {"IntAdd", bytecode::IntAdd},
      {"IntSubtract", bytecode::IntSubtract},
      {"IntMultiply", bytecode::IntMultiply},
      {"IntDivide", bytecode::IntDivide},
      {"IntModulo", bytecode::IntModulo},
      {"FloatAdd", bytecode::FloatAdd},
      {"FloatSubtract", bytecode::FloatSubtract},
      {"FloatMultiply", bytecode::FloatMultiply},
      {"FloatDivide", bytecode::FloatDivide},
      {"ByteAdd", bytecode::ByteAdd},
      {"ByteSubtract", bytecode::ByteSubtract},
      {"ByteMultiply", bytecode::ByteMultiply},
      {"ByteDivide", bytecode::ByteDivide},
      {"ByteModulo", bytecode::ByteModulo},

      // Binary logical operations
      {"BoolAnd", bytecode::BoolAnd},
      {"BoolOr", bytecode::BoolOr},
      {"BoolXor", bytecode::BoolXor},
      {"IntAnd", bytecode::IntAnd},
      {"IntOr", bytecode::IntOr},
      {"IntXor", bytecode::IntXor},
      {"IntLeftShift", bytecode::IntLeftShift},
      {"IntRightShift", bytecode::IntRightShift},
      {"ByteAnd", bytecode::ByteAnd},
      {"ByteOr", bytecode::ByteOr},
      {"ByteXor", bytecode::ByteXor},
      {"ByteLeftShift", bytecode::ByteLeftShift},
      {"ByteRightShift", bytecode::ByteRightShift},

      // Comparison operations
      {"IntEqual", bytecode::IntEqual},
      {"IntNotEqual", bytecode::IntNotEqual},
      {"IntLessThan", bytecode::IntLessThan},
      {"IntLessEqual", bytecode::IntLessEqual},
      {"IntGreaterThan", bytecode::IntGreaterThan},
      {"IntGreaterEqual", bytecode::IntGreaterEqual},
      {"FloatEqual", bytecode::FloatEqual},
      {"FloatNotEqual", bytecode::FloatNotEqual},
      {"FloatLessThan", bytecode::FloatLessThan},
      {"FloatLessEqual", bytecode::FloatLessEqual},
      {"FloatGreaterThan", bytecode::FloatGreaterThan},
      {"FloatGreaterEqual", bytecode::FloatGreaterEqual},
      {"ByteEqual", bytecode::ByteEqual},
      {"ByteNotEqual", bytecode::ByteNotEqual},
      {"ByteLessThan", bytecode::ByteLessThan},
      {"ByteLessEqual", bytecode::ByteLessEqual},
      {"ByteGreaterThan", bytecode::ByteGreaterThan},
      {"ByteGreaterEqual", bytecode::ByteGreaterEqual},

      // String operations
      {"StringConcat", bytecode::StringConcat},
      {"StringLength", bytecode::StringLength},
      {"StringSubstring", bytecode::StringSubstring},
      {"StringCompare", bytecode::StringCompare},
      {"StringToInt", bytecode::StringToInt},
      {"StringToFloat", bytecode::StringToFloat},
      {"IntToString", bytecode::IntToString},
      {"FloatToString", bytecode::FloatToString},

      // Type conversions
      {"IntToFloat", bytecode::IntToFloat},
      {"FloatToInt", bytecode::FloatToInt},
      {"ByteToInt", bytecode::ByteToInt},
      {"CharToByte", bytecode::CharToByte},
      {"ByteToChar", bytecode::ByteToChar},
      {"BoolToByte", bytecode::BoolToByte},

      // Indirect calls
      {"CallIndirect", bytecode::CallIndirect},

      // Object operations
      {"Unwrap", bytecode::Unwrap},
      {"NullCoalesce", bytecode::NullCoalesce},
      {"IsNull", bytecode::IsNull},

      // I/O operations
      {"Print", bytecode::Print},
      {"PrintLine", bytecode::PrintLine},
      {"ReadLine", bytecode::ReadLine},
      {"ReadChar", bytecode::ReadChar},
      {"ReadInt", bytecode::ReadInt},
      {"ReadFloat", bytecode::ReadFloat},

      // Time operations
      {"UnixTime", bytecode::UnixTime},
      {"UnixTimeMs", bytecode::UnixTimeMs},
      {"UnixTimeNs", bytecode::UnixTimeNs},
      {"NanoTime", bytecode::NanoTime},
      {"FormatDateTime", bytecode::FormatDateTime},
      {"ParseDateTime", bytecode::ParseDateTime},

      // File system operations
      {"FileExists", bytecode::FileExists},
      {"DirectoryExists", bytecode::DirectoryExists},
      {"CreateDirectory", bytecode::CreateDir},
      {"DeleteFile", bytecode::DeleteFileByName},
      {"DeleteDirectory", bytecode::DeleteDir},
      {"MoveFile", bytecode::MoveFileByName},
      {"CopyFile", bytecode::CopyFileByName},
      {"ListDirectory", bytecode::ListDir},
      {"GetCurrentDirectory", bytecode::GetCurrentDir},
      {"ChangeDirectory", bytecode::ChangeDir},

      // System operations
      {"SleepMs", bytecode::SleepMs},
      {"SleepNs", bytecode::SleepNs},
      {"Exit", bytecode::Exit},
      {"GetProcessId", bytecode::GetProcessId},
      {"GetEnvironmentVar", bytecode::GetEnvironmentVar},
      {"SetEnvironmentVar", bytecode::SetEnvironmentVar},

      // Random operations
      {"Random", bytecode::Random},
      {"RandomRange", bytecode::RandomRange},
      {"RandomFloat", bytecode::RandomFloat},
      {"RandomFloatRange", bytecode::RandomFloatRange},
      {"SeedRandom", bytecode::SeedRandom},

      // Memory and system info
      {"GetMemoryUsage", bytecode::GetMemoryUsage},
      {"GetPeakMemoryUsage", bytecode::GetPeakMemoryUsage},
      {"ForceGarbageCollection", bytecode::ForceGarbageCollection},
      {"GetProcessorCount", bytecode::GetProcessorCount},

      // OS info
      {"GetOsName", bytecode::GetOsName},
      {"GetOsVersion", bytecode::GetOsVersion},
      {"GetArchitecture", bytecode::GetArchitecture},
      {"GetUserName", bytecode::GetUsername},
      {"GetHomeDirectory", bytecode::GetHomeDir},

      // Type operations
      {"TypeOf", bytecode::TypeOf},
  };
  return kMap;
}

const std::unordered_map<std::string, StringCommandFunc>& GetStringCommands() {
  static const std::unordered_map<std::string, StringCommandFunc> kMap = {
      {"PushString", bytecode::PushString},
      {"Call", bytecode::Call},
      {"CallVirtual", bytecode::CallVirtual},
      {"CallConstructor", bytecode::CallConstructor},
      {"GetVTable", bytecode::GetVTable},
      {"SetVTable", bytecode::SetVTable},
      {"SafeCall", bytecode::SafeCall},
      {"IsType", bytecode::IsType},
      {"SizeOf", bytecode::SizeOf},
  };
  return kMap;
}

const std::unordered_map<std::string, IntegerCommandFunc>& GetIntegerCommands() {
  static const std::unordered_map<std::string, IntegerCommandFunc> kMap = {
      {"PushInt", bytecode::PushInt},
      {"Rotate", bytecode::Rotate},
      {"LoadLocal",
       [](PassedExecutionData& data, int64_t value) { return bytecode::LoadLocal(data, static_cast<size_t>(value)); }},
      {"SetLocal",
       [](PassedExecutionData& data, int64_t value) { return bytecode::SetLocal(data, static_cast<size_t>(value)); }},
      {"LoadStatic",
       [](PassedExecutionData& data, int64_t value) { return bytecode::LoadStatic(data, static_cast<size_t>(value)); }},
      {"SetStatic",
       [](PassedExecutionData& data, int64_t value) { return bytecode::SetStatic(data, static_cast<size_t>(value)); }},
      {"GetField",
       [](PassedExecutionData& data, int64_t value) { return bytecode::GetField(data, static_cast<size_t>(value)); }},
      {"SetField",
       [](PassedExecutionData& data, int64_t value) { return bytecode::SetField(data, static_cast<size_t>(value)); }},
  };
  return kMap;
}

const std::unordered_map<std::string, FloatCommandFunc>& GetFloatCommands() {
  static const std::unordered_map<std::string, FloatCommandFunc> kMap = {
      {"PushFloat", bytecode::PushFloat},
  };
  return kMap;
}

const std::unordered_map<std::string, BooleanCommandFunc>& GetBooleanCommands() {
  static const std::unordered_map<std::string, BooleanCommandFunc> kMap = {
      {"PushBool", bytecode::PushBool},
  };
  return kMap;
}

// Special commands that need type conversion
const std::unordered_map<std::string, IntegerCommandFunc>& GetCharCommands() {
  static const std::unordered_map<std::string, IntegerCommandFunc> kMap = {
      {"PushChar",
       [](PassedExecutionData& data, int64_t value) { return bytecode::PushChar(data, static_cast<char>(value)); }},
  };
  return kMap;
}

const std::unordered_map<std::string, IntegerCommandFunc>& GetByteCommands() {
  static const std::unordered_map<std::string, IntegerCommandFunc> kMap = {
      {"PushByte",
       [](PassedExecutionData& data, int64_t value) { return bytecode::PushByte(data, static_cast<uint8_t>(value)); }},
  };
  return kMap;
}

} // namespace

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateSimpleCommandByName(const std::string& name) {
  const auto& map = GetSimpleCommands();
  try {
    return std::make_unique<Command<SimpleCommandFunc>>(map.at(name));
  } catch (const std::out_of_range&) {
    return std::unexpected(std::out_of_range("Command not found: " + name));
  }
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateStringCommandByName(const std::string& name,
                                                                                         const std::string& value) {
  const auto& map = GetStringCommands();
  try {
    return CreateCommandWithArg(map.at(name), value);
  } catch (const std::out_of_range&) {
    return std::unexpected(std::out_of_range("Command not found: " + name));
  }
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateIntegerCommandByName(const std::string& name,
                                                                                          const int64_t value) {
  // Check in regular integer commands first
  {
    const auto& map = GetIntegerCommands();
    try {
      return CreateCommandWithArg(map.at(name), value);
    } catch (const std::out_of_range&) {
      // Continue to next map
    }
  }

  // Check in char commands
  {
    const auto& map = GetCharCommands();
    try {
      return CreateCommandWithArg(map.at(name), value);
    } catch (const std::out_of_range&) {
      // Continue to next map
    }
  }

  // Check in byte commands
  {
    const auto& map = GetByteCommands();
    try {
      return CreateCommandWithArg(map.at(name), value);
    } catch (const std::out_of_range&) {
      // Continue to next map
    }
  }

  return std::unexpected(std::out_of_range("Command not found: " + name));
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateFloatCommandByName(const std::string& name,
                                                                                        const double value) {
  const auto& map = GetFloatCommands();
  try {
    return CreateCommandWithArg(map.at(name), value);
  } catch (const std::out_of_range&) {
    return std::unexpected(std::out_of_range("Command not found: " + name));
  }
}

std::expected<std::unique_ptr<IExecutable>, std::out_of_range> CreateBooleanCommandByName(const std::string& name,
                                                                                          const bool value) {
  const auto& map = GetBooleanCommands();
  try {
    return CreateCommandWithArg(map.at(name), value);
  } catch (const std::out_of_range&) {
    return std::unexpected(std::out_of_range("Command not found: " + name));
  }
}

} // namespace ovum::vm::execution_tree
