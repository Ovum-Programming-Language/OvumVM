#ifndef BYTECODE_COMMANDS_HPP
#define BYTECODE_COMMANDS_HPP

#include <expected>
#include <stdexcept>
#include <stack>
#include <vector>
#include <string>
#include <cstdint>

#include "ExecutionConcepts.hpp"
#include "ExecutionResult.hpp"
#include "PassedExecutionData.hpp"


namespace ovum::vm::execution_tree::bytecode {

std::expected<ExecutionResult, std::runtime_error> PushInt(PassedExecutionData& data, int64_t value);
std::expected<ExecutionResult, std::runtime_error> PushFloat(PassedExecutionData& data, double value);
std::expected<ExecutionResult, std::runtime_error> PushBool(PassedExecutionData& data, bool value);
std::expected<ExecutionResult, std::runtime_error> PushChar(PassedExecutionData& data, char value);
std::expected<ExecutionResult, std::runtime_error> PushByte(PassedExecutionData& data, uint8_t value);
std::expected<ExecutionResult, std::runtime_error> PushString(PassedExecutionData& data, const std::string& value);
std::expected<ExecutionResult, std::runtime_error> PushNull(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> Pop(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> Dup(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> Swap(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> Rotate(PassedExecutionData& data, uint64_t n);

std::expected<ExecutionResult, std::runtime_error> LoadLocal(PassedExecutionData& data, size_t index);
std::expected<ExecutionResult, std::runtime_error> SetLocal(PassedExecutionData& data, size_t index);
std::expected<ExecutionResult, std::runtime_error> LoadStatic(PassedExecutionData& data, size_t index);
std::expected<ExecutionResult, std::runtime_error> SetStatic(PassedExecutionData& data, size_t index);

std::expected<ExecutionResult, std::runtime_error> ArrayGet(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ArraySet(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntSubtract(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntMultiply(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntDivide(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntModulo(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntNegate(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntIncrement(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntDecrement(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> FloatAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatSubtract(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatMultiply(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatDivide(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatNegate(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatSqrt(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> ByteAdd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteSubtract(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteMultiply(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteDivide(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteModulo(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteNegate(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteIncrement(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteDecrement(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntNotEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntLessThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntLessEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntGreaterThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntGreaterEqual(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> FloatEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatNotEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatLessThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatLessEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatGreaterThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatGreaterEqual(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> ByteEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteNotEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteLessThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteLessEqual(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteGreaterThan(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteGreaterEqual(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> BoolAnd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolOr(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolNot(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolXor(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntAnd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntOr(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntXor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntNot(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntLeftShift(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntRightShift(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> ByteAnd(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteOr(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteXor(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteNot(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteLeftShift(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteRightShift(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> StringConcat(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringLength(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringSubstring(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringCompare(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringToInt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> StringToFloat(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IntToString(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatToString(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> IntToFloat(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FloatToInt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteToInt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CharToByte(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ByteToChar(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> BoolToByte(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> Call(PassedExecutionData& data, const std::string& function);
std::expected<ExecutionResult, std::runtime_error> CallIndirect(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CallVirtual(PassedExecutionData& data, const std::string& method);
std::expected<ExecutionResult, std::runtime_error> Return(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> GetField(PassedExecutionData& data, size_t number);
std::expected<ExecutionResult, std::runtime_error> SetField(PassedExecutionData& data, size_t number);
std::expected<ExecutionResult, std::runtime_error> CallConstructor(PassedExecutionData& data, const std::string& constructor);
std::expected<ExecutionResult, std::runtime_error> Unwrap(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetVTable(PassedExecutionData& data, const std::string& class_name);
std::expected<ExecutionResult, std::runtime_error> SetVTable(PassedExecutionData& data, const std::string& class_name);

std::expected<ExecutionResult, std::runtime_error> SafeCall(PassedExecutionData& data, const std::string& method);
std::expected<ExecutionResult, std::runtime_error> NullCoalesce(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IsNull(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> Print(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> PrintLine(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ReadLine(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ReadChar(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ReadInt(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ReadFloat(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> UnixTime(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> UnixTimeMs(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> UnixTimeNs(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> NanoTime(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FormatDateTime(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ParseDateTime(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> OpenFile(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CloseFile(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> FileExists(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> DirectoryExists(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CreateDirectory(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> DeleteFile(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> DeleteDirectory(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> MoveFile(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> CopyFile(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ListDirectory(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetCurrentDirectory(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ChangeDirectory(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> SleepMs(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> SleepNs(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> Exit(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetProcessId(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetEnvironmentVariable(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> SetEnvironmentVariable(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> Random(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> RandomRange(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> RandomFloat(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> RandomFloatRange(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> SeedRandom(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> GetMemoryUsage(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetPeakMemoryUsage(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> ForceGarbageCollection(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetProcessorCount(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> GetOsName(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetOsVersion(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetArchitecture(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetUserName(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> GetHomeDirectory(PassedExecutionData& data);

std::expected<ExecutionResult, std::runtime_error> TypeOf(PassedExecutionData& data);
std::expected<ExecutionResult, std::runtime_error> IsType(PassedExecutionData& data, const std::string& type);
std::expected<ExecutionResult, std::runtime_error> SizeOf(PassedExecutionData& data, const std::string& type);

} //namespace ovum::vm::execution_tree

#endif // BYTECODE_COMMANDS_HPP
