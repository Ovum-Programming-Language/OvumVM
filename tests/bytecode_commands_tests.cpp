#include "test_suites/BuiltinTestSuite.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <thread>

#include "lib/execution_tree/Command.hpp"
#include "lib/execution_tree/ExecutionResult.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/runtime/Variable.hpp"

using ovum::vm::execution_tree::Command;
using ovum::vm::execution_tree::ExecutionResult;
using ovum::vm::runtime::GetDataPointer;
using ovum::vm::runtime::Variable;

namespace {
std::unique_ptr<ovum::vm::execution_tree::Function> MakeStubFunction(
    const std::string& name,
    size_t arity,
    std::function<std::expected<ExecutionResult, std::runtime_error>(ovum::vm::execution_tree::PassedExecutionData&)>
        fn) {
  auto cmd = std::make_unique<Command<decltype(fn)>>(std::move(fn));
  return std::make_unique<ovum::vm::execution_tree::Function>(name, arity, std::move(cmd));
}
} // namespace

TEST_F(BuiltinTestSuite, StackManipulationCommands) {
  constexpr int64_t kTen = 10;
  constexpr int64_t kOne = 1;
  constexpr int64_t kTwo = 2;
  constexpr int64_t kThree = 3;
  constexpr int64_t kBadRotate = 0;

  auto push_int = MakeIntCmd("PushInt", kTen);
  ASSERT_TRUE(push_int);
  EXPECT_TRUE(push_int->Execute(data_).has_value());

  auto dup = MakeSimple("Dup");
  ASSERT_TRUE(dup);
  EXPECT_TRUE(dup->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kTen);
  EXPECT_EQ(PopInt(), kTen);

  PushInt(kOne);
  PushInt(kTwo);
  auto swap = MakeSimple("Swap");
  ASSERT_TRUE(swap);
  EXPECT_TRUE(swap->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kOne);
  EXPECT_EQ(PopInt(), kTwo);

  PushInt(kOne);
  PushInt(kTwo);
  PushInt(kThree);
  auto rotate = MakeIntCmd("Rotate", kThree);
  ASSERT_TRUE(rotate);
  EXPECT_TRUE(rotate->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kThree);
  EXPECT_EQ(PopInt(), kTwo);
  EXPECT_EQ(PopInt(), kOne);

  auto bad_rotate = MakeIntCmd("Rotate", kBadRotate);
  ASSERT_TRUE(bad_rotate);
  EXPECT_FALSE(bad_rotate->Execute(data_).has_value());
}

TEST_F(BuiltinTestSuite, PushCommandsUseFactory) {
  constexpr double kFloat = 1.5;
  constexpr bool kBool = true;
  constexpr char kChar = 'Z';
  constexpr uint8_t kByte = 0xAB;

  auto push_float = MakeFloatCmd("PushFloat", kFloat);
  ASSERT_TRUE(push_float);
  EXPECT_TRUE(push_float->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kFloat);

  auto push_bool = MakeBoolCmd("PushBool", kBool);
  ASSERT_TRUE(push_bool);
  EXPECT_TRUE(push_bool->Execute(data_).has_value());
  EXPECT_EQ(PopBool(), kBool);

  auto push_char = MakeIntCmd("PushChar", static_cast<int64_t>(kChar));
  ASSERT_TRUE(push_char);
  EXPECT_TRUE(push_char->Execute(data_).has_value());
  EXPECT_EQ(PopChar(), kChar);

  auto push_byte = MakeIntCmd("PushByte", kByte);
  ASSERT_TRUE(push_byte);
  EXPECT_TRUE(push_byte->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), kByte);
}

TEST_F(BuiltinTestSuite, LocalAndStaticOperations) {
  constexpr int64_t kLocalIndex = 2;
  constexpr int64_t kLocalValue = 7;
  constexpr int64_t kStaticIndex = 1;
  constexpr int64_t kStaticValue = 9;
  constexpr size_t kExpectedLocalSize = 3;
  constexpr size_t kExpectedStaticSize = 2;

  auto set_local = MakeIntCmd("SetLocal", kLocalIndex);
  PushInt(kLocalValue);
  ASSERT_TRUE(set_local);
  EXPECT_TRUE(set_local->Execute(data_).has_value());
  ASSERT_EQ(memory_.stack_frames.top().local_variables.size(), kExpectedLocalSize);
  EXPECT_EQ(std::get<int64_t>(memory_.stack_frames.top().local_variables[kLocalIndex]), kLocalValue);

  auto load_local = MakeIntCmd("LoadLocal", kLocalIndex);
  ASSERT_TRUE(load_local);
  EXPECT_TRUE(load_local->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kLocalValue);

  auto set_static = MakeIntCmd("SetStatic", kStaticIndex);
  PushInt(kStaticValue);
  ASSERT_TRUE(set_static);
  EXPECT_TRUE(set_static->Execute(data_).has_value());
  ASSERT_EQ(memory_.global_variables.size(), kExpectedStaticSize);
  EXPECT_EQ(std::get<int64_t>(memory_.global_variables[kStaticIndex]), kStaticValue);

  auto load_static = MakeIntCmd("LoadStatic", kStaticIndex);
  ASSERT_TRUE(load_static);
  EXPECT_TRUE(load_static->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kStaticValue);
}

TEST_F(BuiltinTestSuite, PopAndReturnCommands) {
  constexpr int64_t kValue = 42;

  PushInt(kValue);
  auto pop_cmd = MakeSimple("Pop");
  ASSERT_TRUE(pop_cmd);
  EXPECT_TRUE(pop_cmd->Execute(data_).has_value());
  EXPECT_TRUE(memory_.machine_stack.empty());

  auto ret_cmd = MakeSimple("Return");
  ASSERT_TRUE(ret_cmd);
  auto result = ret_cmd->Execute(data_);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), ExecutionResult::kReturn);
}

TEST_F(BuiltinTestSuite, IntegerArithmeticCommands) {
  constexpr int64_t kAddLeft = 8;
  constexpr int64_t kAddRight = 4;
  constexpr int64_t kMulLeft = 6;
  constexpr int64_t kMulRight = 7;
  constexpr int64_t kDivLeft = 20;
  constexpr int64_t kDivRight = 5;
  constexpr int64_t kDivZero = 0;
  constexpr int64_t kModLeft = 20;
  constexpr int64_t kModRight = 6;
  constexpr int64_t kNegateValue = -5;
  constexpr int64_t kIncValue = 5;
  constexpr int64_t kDecValue = 5;

  PushInt(kAddLeft);
  PushInt(kAddRight);
  auto add = MakeSimple("IntAdd");
  ASSERT_TRUE(add);
  EXPECT_TRUE(add->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kAddLeft + kAddRight);

  PushInt(kAddRight);
  PushInt(kAddLeft);
  auto sub = MakeSimple("IntSubtract");
  ASSERT_TRUE(sub);
  EXPECT_TRUE(sub->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kAddLeft - kAddRight);

  PushInt(kMulRight);
  PushInt(kMulLeft);
  auto mul = MakeSimple("IntMultiply");
  ASSERT_TRUE(mul);
  EXPECT_TRUE(mul->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kMulLeft * kMulRight);

  PushInt(kDivRight);
  PushInt(kDivLeft);
  auto div = MakeSimple("IntDivide");
  ASSERT_TRUE(div);
  EXPECT_TRUE(div->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kDivLeft / kDivRight);

  PushInt(kDivZero);
  PushInt(kDivLeft);
  EXPECT_FALSE(div->Execute(data_).has_value());

  PushInt(kModRight);
  PushInt(kModLeft);
  auto mod = MakeSimple("IntModulo");
  ASSERT_TRUE(mod);
  EXPECT_TRUE(mod->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kModLeft % kModRight);

  PushInt(kNegateValue);
  auto negate = MakeSimple("IntNegate");
  ASSERT_TRUE(negate);
  EXPECT_TRUE(negate->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), -kNegateValue);

  PushInt(kIncValue);
  auto inc = MakeSimple("IntIncrement");
  ASSERT_TRUE(inc);
  EXPECT_TRUE(inc->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kIncValue + 1);

  PushInt(kDecValue);
  auto dec = MakeSimple("IntDecrement");
  ASSERT_TRUE(dec);
  EXPECT_TRUE(dec->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kDecValue - 1);
}

TEST_F(BuiltinTestSuite, FloatArithmeticCommands) {
  constexpr double kAddLeft = 3.0;
  constexpr double kAddRight = 2.0;
  constexpr double kDivLeft = 5.0;
  constexpr double kDivRight = 2.0;
  constexpr double kSqrtValue = 4.0;
  constexpr double kNegative = -1.0;
  constexpr double kDivZero = 0.0;

  PushFloat(kAddLeft);
  PushFloat(kAddRight);
  auto add = MakeSimple("FloatAdd");
  ASSERT_TRUE(add);
  EXPECT_TRUE(add->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kAddLeft + kAddRight);

  PushFloat(kDivRight);
  PushFloat(kDivLeft);
  auto div = MakeSimple("FloatDivide");
  ASSERT_TRUE(div);
  EXPECT_TRUE(div->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kDivLeft / kDivRight);

  PushFloat(kSqrtValue);
  auto sqrt_cmd = MakeSimple("FloatSqrt");
  ASSERT_TRUE(sqrt_cmd);
  EXPECT_TRUE(sqrt_cmd->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), std::sqrt(kSqrtValue));

  PushFloat(kNegative);
  EXPECT_FALSE(sqrt_cmd->Execute(data_).has_value());

  PushFloat(kDivZero);
  PushFloat(kDivLeft);
  EXPECT_FALSE(div->Execute(data_).has_value());
}

TEST_F(BuiltinTestSuite, FloatExtendedOperations) {
  constexpr double kSubLeft = 5.0;
  constexpr double kSubRight = 1.5;
  constexpr double kMulLeft = 2.0;
  constexpr double kMulRight = 3.0;
  constexpr double kNegValue = -4.0;
  PushFloat(kSubRight);
  PushFloat(kSubLeft);
  auto sub = MakeSimple("FloatSubtract");
  ASSERT_TRUE(sub);
  EXPECT_TRUE(sub->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kSubLeft - kSubRight);

  PushFloat(kMulLeft);
  PushFloat(kMulRight);
  auto mul = MakeSimple("FloatMultiply");
  ASSERT_TRUE(mul);
  EXPECT_TRUE(mul->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kMulLeft * kMulRight);

  PushFloat(kNegValue);
  auto neg = MakeSimple("FloatNegate");
  ASSERT_TRUE(neg);
  EXPECT_TRUE(neg->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), -kNegValue);
}

TEST_F(BuiltinTestSuite, ByteArithmeticCommands) {
  constexpr uint8_t kInitial = 10;
  constexpr uint8_t kAddLeft = 5;
  constexpr uint8_t kAddRight = 3;
  constexpr uint8_t kSubLeft = 9;
  constexpr uint8_t kSubRight = 4;
  constexpr uint8_t kDivLeft = 9;
  constexpr uint8_t kDivRight = 3;
  constexpr uint8_t kDivZero = 0;

  auto push_byte = MakeIntCmd("PushByte", kInitial);
  ASSERT_TRUE(push_byte);
  EXPECT_TRUE(push_byte->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), kInitial);

  PushByte(kAddLeft);
  PushByte(kAddRight);
  auto add = MakeSimple("ByteAdd");
  ASSERT_TRUE(add);
  EXPECT_TRUE(add->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kAddLeft + kAddRight));

  PushByte(kSubRight);
  PushByte(kSubLeft);
  auto sub = MakeSimple("ByteSubtract");
  ASSERT_TRUE(sub);
  EXPECT_TRUE(sub->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kSubLeft - kSubRight));

  PushByte(kDivRight);
  PushByte(kDivLeft);
  auto div = MakeSimple("ByteDivide");
  ASSERT_TRUE(div);
  EXPECT_TRUE(div->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kDivLeft / kDivRight));

  PushByte(kDivZero);
  PushByte(kDivLeft);
  EXPECT_FALSE(div->Execute(data_).has_value());
}

TEST_F(BuiltinTestSuite, ByteExtendedOperations) {
  constexpr uint8_t kMulLeft = 3;
  constexpr uint8_t kMulRight = 2;
  constexpr uint8_t kModLeft = 9;
  constexpr uint8_t kModRight = 5;
  constexpr uint8_t kNegValue = 5;
  constexpr uint8_t kIncValue = 4;
  constexpr uint8_t kDecValue = 4;
  constexpr uint8_t kNotValue = 0b1010;
  constexpr uint8_t kOrLeft = 0b0101;
  constexpr uint8_t kOrRight = 0b0011;
  constexpr uint8_t kXorLeft = 0b0110;
  constexpr uint8_t kXorRight = 0b0011;
  constexpr uint8_t kAndLeft = 0b1111;
  constexpr uint8_t kAndRight = 0b0011;
  constexpr uint8_t kShiftRightValue = 0b1000;
  constexpr uint8_t kShiftRightBy = 1;

  PushByte(kMulRight);
  PushByte(kMulLeft);
  auto mul = MakeSimple("ByteMultiply");
  ASSERT_TRUE(mul);
  EXPECT_TRUE(mul->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kMulLeft * kMulRight));

  PushByte(kModRight);
  PushByte(kModLeft);
  auto mod = MakeSimple("ByteModulo");
  ASSERT_TRUE(mod);
  EXPECT_TRUE(mod->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kModLeft % kModRight));

  PushByte(kNegValue);
  auto neg = MakeSimple("ByteNegate");
  ASSERT_TRUE(neg);
  EXPECT_TRUE(neg->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(-static_cast<int>(kNegValue)));

  PushByte(kIncValue);
  auto inc = MakeSimple("ByteIncrement");
  ASSERT_TRUE(inc);
  EXPECT_TRUE(inc->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kIncValue + 1));

  PushByte(kDecValue);
  auto dec = MakeSimple("ByteDecrement");
  ASSERT_TRUE(dec);
  EXPECT_TRUE(dec->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kDecValue - 1));

  PushByte(kNotValue);
  auto not_cmd = MakeSimple("ByteNot");
  ASSERT_TRUE(not_cmd);
  EXPECT_TRUE(not_cmd->Execute(data_).has_value());
  PopByte();

  PushByte(kOrLeft);
  PushByte(kOrRight);
  auto or_cmd = MakeSimple("ByteOr");
  ASSERT_TRUE(or_cmd);
  EXPECT_TRUE(or_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kOrLeft | kOrRight));

  PushByte(kXorRight);
  PushByte(kXorLeft);
  auto xor_cmd = MakeSimple("ByteXor");
  ASSERT_TRUE(xor_cmd);
  EXPECT_TRUE(xor_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kXorLeft ^ kXorRight));

  PushByte(kAndLeft);
  PushByte(kAndRight);
  auto and_cmd = MakeSimple("ByteAnd");
  ASSERT_TRUE(and_cmd);
  EXPECT_TRUE(and_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kAndLeft & kAndRight));

  PushByte(kShiftRightBy);
  PushByte(kShiftRightValue);
  auto rshift = MakeSimple("ByteRightShift");
  ASSERT_TRUE(rshift);
  EXPECT_TRUE(rshift->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kShiftRightValue >> kShiftRightBy));
}

TEST_F(BuiltinTestSuite, ComparisonCommands) {
  constexpr int64_t kIntEqualValue = 5;
  constexpr int64_t kIntLtLeft = 7;
  constexpr int64_t kIntLtRight = 5;
  constexpr int64_t kIntLeValue = 5;
  constexpr double kFloatGtLeft = 2.0;
  constexpr double kFloatGtRight = 1.5;
  constexpr double kFloatEqValue = 2.0;
  constexpr double kFloatLtLeft = 2.0;
  constexpr double kFloatLtRight = 1.0;
  constexpr double kFloatLeLeft = 2.5;
  constexpr double kFloatLeRight = 2.0;
  constexpr double kFloatGeLeft = 3.0;
  constexpr double kFloatGeRight = 2.0;
  constexpr uint8_t kByteLeLeft = 7;
  constexpr uint8_t kByteLeRight = 5;
  constexpr uint8_t kByteEqValue = 8;
  constexpr uint8_t kByteGeValue = 9;
  constexpr uint8_t kByteNeLeft = 2;
  constexpr uint8_t kByteNeRight = 1;
  constexpr int64_t kIntNeLeft = 3;
  constexpr int64_t kIntNeRight = 2;
  constexpr int64_t kIntGtLeft = 4;
  constexpr int64_t kIntGtRight = 1;
  constexpr int64_t kIntGeValue = 4;
  constexpr double kFloatNeValue = 1.0;
  constexpr uint8_t kByteLtLeft = 2;
  constexpr uint8_t kByteLtRight = 1;
  constexpr uint8_t kByteGtLeft = 2;
  constexpr uint8_t kByteGtRight = 1;
  PushInt(kIntEqualValue);
  PushInt(kIntEqualValue);
  auto int_eq = MakeSimple("IntEqual");
  ASSERT_TRUE(int_eq);
  EXPECT_TRUE(int_eq->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushInt(kIntLtLeft);
  PushInt(kIntLtRight);
  auto int_lt = MakeSimple("IntLessThan");
  ASSERT_TRUE(int_lt);
  EXPECT_TRUE(int_lt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushInt(kIntLeValue);
  PushInt(kIntLeValue);
  auto int_le = MakeSimple("IntLessEqual");
  ASSERT_TRUE(int_le);
  EXPECT_TRUE(int_le->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatGtRight);
  PushFloat(kFloatGtLeft);
  auto float_gt = MakeSimple("FloatGreaterThan");
  ASSERT_TRUE(float_gt);
  EXPECT_TRUE(float_gt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatEqValue);
  PushFloat(kFloatEqValue);
  auto float_eq = MakeSimple("FloatEqual");
  ASSERT_TRUE(float_eq);
  EXPECT_TRUE(float_eq->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatLtLeft);
  PushFloat(kFloatLtRight);
  auto float_lt = MakeSimple("FloatLessThan");
  ASSERT_TRUE(float_lt);
  EXPECT_TRUE(float_lt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatLeLeft);
  PushFloat(kFloatLeRight);
  auto float_le = MakeSimple("FloatLessEqual");
  ASSERT_TRUE(float_le);
  EXPECT_TRUE(float_le->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatGeRight);
  PushFloat(kFloatGeLeft);
  auto float_ge = MakeSimple("FloatGreaterEqual");
  ASSERT_TRUE(float_ge);
  EXPECT_TRUE(float_ge->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushByte(kByteLeLeft);
  PushByte(kByteLeRight);
  auto byte_le = MakeSimple("ByteLessEqual");
  ASSERT_TRUE(byte_le);
  EXPECT_TRUE(byte_le->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushByte(kByteEqValue);
  PushByte(kByteEqValue);
  auto byte_eq = MakeSimple("ByteEqual");
  ASSERT_TRUE(byte_eq);
  EXPECT_TRUE(byte_eq->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushByte(kByteNeRight);
  PushByte(kByteNeLeft);
  auto byte_ne = MakeSimple("ByteNotEqual");
  ASSERT_TRUE(byte_ne);
  EXPECT_TRUE(byte_ne->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushInt(kIntNeRight);
  PushInt(kIntNeLeft);
  auto int_ne = MakeSimple("IntNotEqual");
  ASSERT_TRUE(int_ne);
  EXPECT_TRUE(int_ne->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushInt(kIntGtRight);
  PushInt(kIntGtLeft);
  auto int_gt = MakeSimple("IntGreaterThan");
  ASSERT_TRUE(int_gt);
  EXPECT_TRUE(int_gt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushInt(kIntGeValue);
  PushInt(kIntGeValue);
  auto int_ge = MakeSimple("IntGreaterEqual");
  ASSERT_TRUE(int_ge);
  EXPECT_TRUE(int_ge->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushFloat(kFloatNeValue);
  PushFloat(kFloatNeValue);
  auto float_ne = MakeSimple("FloatNotEqual");
  ASSERT_TRUE(float_ne);
  EXPECT_TRUE(float_ne->Execute(data_).has_value());
  EXPECT_FALSE(PopBool());

  PushByte(kByteGeValue);
  PushByte(kByteGeValue);
  auto byte_ge = MakeSimple("ByteGreaterEqual");
  ASSERT_TRUE(byte_ge);
  EXPECT_TRUE(byte_ge->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushByte(kByteLtLeft);
  PushByte(kByteLtRight);
  auto byte_lt = MakeSimple("ByteLessThan");
  ASSERT_TRUE(byte_lt);
  EXPECT_TRUE(byte_lt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushByte(kByteGtRight);
  PushByte(kByteGtLeft);
  auto byte_gt = MakeSimple("ByteGreaterThan");
  ASSERT_TRUE(byte_gt);
  EXPECT_TRUE(byte_gt->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());
}

TEST_F(BuiltinTestSuite, BooleanAndBitwiseCommands) {
  constexpr bool kTrue = true;
  constexpr bool kFalse = false;
  constexpr int64_t kIntAndLeft = 0b1010;
  constexpr int64_t kIntAndRight = 0b1100;
  constexpr uint8_t kByteShiftValue = 0b0011;
  constexpr uint8_t kByteShiftBy = 1;

  PushBool(kTrue);
  PushBool(kFalse);
  auto and_cmd = MakeSimple("BoolAnd");
  ASSERT_TRUE(and_cmd);
  EXPECT_TRUE(and_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopBool(), kFalse);

  PushBool(kTrue);
  auto not_cmd = MakeSimple("BoolNot");
  ASSERT_TRUE(not_cmd);
  EXPECT_TRUE(not_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopBool(), kFalse);

  PushBool(kTrue);
  PushBool(kFalse);
  auto or_cmd = MakeSimple("BoolOr");
  ASSERT_TRUE(or_cmd);
  EXPECT_TRUE(or_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopBool(), kTrue);

  PushBool(kTrue);
  PushBool(kTrue);
  auto xor_cmd = MakeSimple("BoolXor");
  ASSERT_TRUE(xor_cmd);
  EXPECT_TRUE(xor_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopBool(), kFalse);

  PushInt(kIntAndLeft);
  PushInt(kIntAndRight);
  auto int_and = MakeSimple("IntAnd");
  ASSERT_TRUE(int_and);
  EXPECT_TRUE(int_and->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kIntAndLeft & kIntAndRight);

  PushByte(kByteShiftBy);
  PushByte(kByteShiftValue);
  auto byte_shift = MakeSimple("ByteLeftShift");
  ASSERT_TRUE(byte_shift);
  EXPECT_TRUE(byte_shift->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kByteShiftValue << kByteShiftBy));
}

TEST_F(BuiltinTestSuite, IntBitwiseAndShiftCommands) {
  constexpr int64_t kOrLeft = 0b1010;
  constexpr int64_t kOrRight = 0b0101;
  constexpr int64_t kXorLeft = 0b1111;
  constexpr int64_t kXorRight = 0b0101;
  constexpr int64_t kNotValue = 0b1111;
  constexpr int64_t kLShiftValue = 1;
  constexpr int64_t kLShiftBy = 3;
  constexpr int64_t kRShiftValue = 8;
  constexpr int64_t kRShiftBy = 1;

  PushInt(kOrLeft);
  PushInt(kOrRight);
  auto or_cmd = MakeSimple("IntOr");
  ASSERT_TRUE(or_cmd);
  EXPECT_TRUE(or_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kOrLeft | kOrRight);

  PushInt(kXorLeft);
  PushInt(kXorRight);
  auto xor_cmd = MakeSimple("IntXor");
  ASSERT_TRUE(xor_cmd);
  EXPECT_TRUE(xor_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kXorLeft ^ kXorRight);

  PushInt(kNotValue);
  auto not_cmd = MakeSimple("IntNot");
  ASSERT_TRUE(not_cmd);
  EXPECT_TRUE(not_cmd->Execute(data_).has_value());
  PopInt();

  PushInt(kLShiftBy);
  PushInt(kLShiftValue);
  auto lshift = MakeSimple("IntLeftShift");
  ASSERT_TRUE(lshift);
  EXPECT_TRUE(lshift->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kLShiftValue << kLShiftBy);

  PushInt(kRShiftBy);
  PushInt(kRShiftValue);
  auto rshift = MakeSimple("IntRightShift");
  ASSERT_TRUE(rshift);
  EXPECT_TRUE(rshift->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kRShiftValue >> kRShiftBy);
}

TEST_F(BuiltinTestSuite, StringOperations) {
  constexpr std::string_view kFirst = "hello";
  constexpr std::string_view kSecond = "world";
  constexpr std::string_view kSubSource = "bytecode";
  constexpr int64_t kSubStart = 4;
  constexpr int64_t kSubLen = 3;
  constexpr std::string_view kCmpLeft = "abc";
  constexpr std::string_view kCmpRight = "abd";
  constexpr std::string_view kConcatExpected = "helloworld";
  constexpr std::string_view kSubExpected = "cod";
  constexpr int64_t kConcatLength = 10;
  constexpr int64_t kCompareLessThan = 0;

  auto second_str = MakeStringCmd("PushString", std::string{kSecond});
  ASSERT_TRUE(second_str);
  EXPECT_TRUE(second_str->Execute(data_).has_value());

  auto push_str = MakeStringCmd("PushString", std::string{kFirst});
  ASSERT_TRUE(push_str);
  EXPECT_TRUE(push_str->Execute(data_).has_value());

  auto concat = MakeSimple("StringConcat");
  ASSERT_TRUE(concat);
  EXPECT_TRUE(concat->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kConcatExpected});

  auto len = MakeSimple("StringLength");
  ASSERT_TRUE(len);
  EXPECT_TRUE(len->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kConcatLength);

  // Prepare substring (string on stack + indices)
  auto substr_src = MakeString(std::string{kSubSource});
  PushInt(kSubLen);   // length
  PushInt(kSubStart); // start
  PushObject(substr_src);
  auto substr = MakeSimple("StringSubstring");
  ASSERT_TRUE(substr);
  EXPECT_TRUE(substr->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kSubExpected});
  PopObject();

  auto cmp_left = MakeString(std::string{kCmpLeft});
  auto cmp_right = MakeString(std::string{kCmpRight});
  PushObject(cmp_right);
  PushObject(cmp_left);
  auto cmp = MakeSimple("StringCompare");
  ASSERT_TRUE(cmp);
  EXPECT_TRUE(cmp->Execute(data_).has_value());
  EXPECT_LT(PopInt(), kCompareLessThan);
}

TEST_F(BuiltinTestSuite, StringAndNumericConversions) {
  constexpr std::string_view kIntString = "123";
  constexpr std::string_view kFloatString = "3.14";
  constexpr int64_t kIntValue = 123;
  constexpr double kFloatValue = 3.14;
  constexpr int64_t kIntToStrValue = 42;
  constexpr double kFloatToStrValue = 2.5;
  constexpr std::string_view kIntToStrExpected = "42";
  constexpr std::string_view kFloatToStrExpected = "2.500000";

  auto push_num_str = MakeString(std::string{kIntString});
  PushObject(push_num_str);
  auto to_int = MakeSimple("StringToInt");
  ASSERT_TRUE(to_int);
  EXPECT_TRUE(to_int->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kIntValue);

  auto push_float_str = MakeString(std::string{kFloatString});
  PushObject(push_float_str);
  auto to_float = MakeSimple("StringToFloat");
  ASSERT_TRUE(to_float);
  EXPECT_TRUE(to_float->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kFloatValue);

  PushInt(kIntToStrValue);
  auto int_to_str = MakeSimple("IntToString");
  ASSERT_TRUE(int_to_str);
  EXPECT_TRUE(int_to_str->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kIntToStrExpected});
  PopObject();

  PushFloat(kFloatToStrValue);
  auto float_to_str = MakeSimple("FloatToString");
  ASSERT_TRUE(float_to_str);
  EXPECT_TRUE(float_to_str->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kFloatToStrExpected});
}

TEST_F(BuiltinTestSuite, NumericConversions) {
  constexpr int64_t kIntValue = 3;
  constexpr double kFloatValue = 7.8;
  constexpr uint8_t kByteValue = 200;
  constexpr char kCharValue = 'A';
  constexpr uint8_t kByteChar = 65;
  constexpr bool kBoolValue = true;

  PushInt(kIntValue);
  auto to_float = MakeSimple("IntToFloat");
  ASSERT_TRUE(to_float);
  EXPECT_TRUE(to_float->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), static_cast<double>(kIntValue));

  PushFloat(kFloatValue);
  auto to_int = MakeSimple("FloatToInt");
  ASSERT_TRUE(to_int);
  EXPECT_TRUE(to_int->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), static_cast<int64_t>(kFloatValue));

  PushByte(kByteValue);
  auto byte_to_int = MakeSimple("ByteToInt");
  ASSERT_TRUE(byte_to_int);
  EXPECT_TRUE(byte_to_int->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kByteValue);

  PushChar(kCharValue);
  auto char_to_byte = MakeSimple("CharToByte");
  ASSERT_TRUE(char_to_byte);
  EXPECT_TRUE(char_to_byte->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kCharValue));

  PushByte(kByteChar);
  auto byte_to_char = MakeSimple("ByteToChar");
  ASSERT_TRUE(byte_to_char);
  EXPECT_TRUE(byte_to_char->Execute(data_).has_value());
  EXPECT_EQ(PopChar(), static_cast<char>(kByteChar));

  PushBool(kBoolValue);
  auto bool_to_byte = MakeSimple("BoolToByte");
  ASSERT_TRUE(bool_to_byte);
  EXPECT_TRUE(bool_to_byte->Execute(data_).has_value());
  EXPECT_EQ(PopByte(), static_cast<uint8_t>(kBoolValue));
}

TEST_F(BuiltinTestSuite, CallAndIndirectCommands) {
  constexpr int64_t kReturnValue = 99;
  constexpr int64_t kInvalidIndex = 999;
  constexpr std::string_view kTargetName = "Target";

  auto stub = MakeStubFunction(std::string{kTargetName}, 0, [kReturnValue](auto& data) {
    data.memory.machine_stack.emplace(kReturnValue);
    return ExecutionResult::kNormal;
  });
  auto idx = function_repo_.Add(std::move(stub));
  ASSERT_TRUE(idx.has_value());

  auto call_cmd = MakeStringCmd("Call", std::string{kTargetName});
  ASSERT_TRUE(call_cmd);
  EXPECT_TRUE(call_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kReturnValue);

  auto indirect_cmd = MakeSimple("CallIndirect");
  PushInt(static_cast<int64_t>(idx.value()));
  ASSERT_TRUE(indirect_cmd);
  EXPECT_TRUE(indirect_cmd->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kReturnValue);

  // Invalid index should fail
  PushInt(kInvalidIndex);
  EXPECT_FALSE(indirect_cmd->Execute(data_).has_value());
}

TEST_F(BuiltinTestSuite, CallVirtualConstructorAndFields) {
  constexpr int64_t kFieldValue = 77;
  constexpr int64_t kVirtualReturn = 321;
  constexpr std::string_view kClassName = "Custom";
  constexpr std::string_view kVirtualDestructorName = "_destructor_<M>";
  constexpr std::string_view kRealDestructorName = "_Custom_destructor_<M>";
  constexpr std::string_view kMethodName = "virt";
  constexpr std::string_view kFieldType = "int";
  constexpr std::string_view kRealMethodName = "real";

  // Create custom vtable with one int field and virtual function mapping
  ovum::vm::runtime::VirtualTable vt(std::string{kClassName},
                                     sizeof(ovum::vm::runtime::ObjectDescriptor) + sizeof(int64_t));
  auto field_offset = vt.AddField(std::string{kFieldType}, sizeof(ovum::vm::runtime::ObjectDescriptor));
  vt.AddFunction(std::string{kMethodName}, std::string{kRealMethodName});
  vt.AddFunction(std::string{kVirtualDestructorName}, std::string{kRealDestructorName});
  auto vt_index = vtable_repo_.Add(std::move(vt));
  ASSERT_TRUE(vt_index.has_value());

  auto func = MakeStubFunction(std::string{kRealMethodName}, 1, [kVirtualReturn](auto& data) {
    data.memory.machine_stack.emplace(static_cast<int64_t>(kVirtualReturn));
    return ExecutionResult::kNormal;
  });
  auto destructor_func =
      MakeStubFunction(std::string{kRealDestructorName}, 1, [](auto& data) { return ExecutionResult::kNormal; });
  auto func_idx = function_repo_.Add(std::move(func));
  ASSERT_TRUE(func_idx.has_value());
  auto destructor_idx = function_repo_.Add(std::move(destructor_func));
  ASSERT_TRUE(destructor_idx.has_value());
  (void) func_idx;
  (void) destructor_idx;

  auto obj_res = memory_manager_.AllocateObject(
      *vtable_repo_.GetByIndex(vt_index.value()).value(), static_cast<uint32_t>(vt_index.value()), data_);
  ASSERT_TRUE(obj_res.has_value());
  void* obj = obj_res.value();

  // SetField: push value then object (object must be on top)
  PushInt(kFieldValue);
  PushObject(obj);
  auto set_field = MakeIntCmd("SetField", static_cast<int64_t>(field_offset));
  ASSERT_TRUE(set_field);
  EXPECT_TRUE(set_field->Execute(data_).has_value());

  PushObject(obj);
  auto get_field = MakeIntCmd("GetField", static_cast<int64_t>(field_offset));
  ASSERT_TRUE(get_field);
  ASSERT_TRUE(get_field->Execute(data_).has_value());
  ASSERT_FALSE(memory_.machine_stack.empty());
  EXPECT_EQ(PopInt(), kFieldValue);

  PushObject(obj);
  auto call_virtual = MakeStringCmd("CallVirtual", std::string{kMethodName});
  ASSERT_TRUE(call_virtual);
  ASSERT_TRUE(call_virtual->Execute(data_).has_value());
  ASSERT_FALSE(memory_.machine_stack.empty());
  EXPECT_EQ(PopInt(), kVirtualReturn);

  auto ctor_func = MakeStubFunction(std::string{kClassName}, 1, [](auto& data) {
    // Mimic constructor returning the created object
    auto obj_var = data.memory.stack_frames.top().local_variables[0];
    data.memory.machine_stack.emplace(obj_var);
    return ExecutionResult::kNormal;
  });
  ASSERT_TRUE(function_repo_.Add(std::move(ctor_func)).has_value());
  auto ctor_cmd = MakeStringCmd("CallConstructor", std::string{kClassName});
  ASSERT_TRUE(ctor_cmd);
  ASSERT_TRUE(ctor_cmd->Execute(data_).has_value());
  ASSERT_FALSE(memory_.machine_stack.empty());
  EXPECT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
  PopObject();

  auto get_vt = MakeStringCmd("GetVTable", std::string{kClassName});
  ASSERT_TRUE(get_vt);
  ASSERT_TRUE(get_vt->Execute(data_).has_value());
  ASSERT_FALSE(memory_.machine_stack.empty());
  EXPECT_EQ(PopInt(), static_cast<int64_t>(vt_index.value()));

  PushObject(obj);
  auto set_vt = MakeStringCmd("SetVTable", std::string{kClassName});
  ASSERT_TRUE(set_vt);
  ASSERT_TRUE(set_vt->Execute(data_).has_value());
  PopObject();
}

TEST_F(BuiltinTestSuite, NullableAndSafeCallCommands) {
  constexpr std::string_view kInnerValue = "hi";
  constexpr int64_t kCoalesceInt = 0;
  constexpr int64_t kSafeReturnValue = 55;
  constexpr std::string_view kSafeMethodName = "SafeMethod";

  auto push_null = MakeSimple("PushNull");
  ASSERT_TRUE(push_null);
  EXPECT_TRUE(push_null->Execute(data_).has_value());
  ExpectTopNullableHasValue(false);

  auto nullable_obj = MakeNullable(nullptr);
  PushObject(nullable_obj);
  auto coalesce = MakeSimple("NullCoalesce");
  ASSERT_TRUE(coalesce);
  EXPECT_TRUE(coalesce->Execute(data_).has_value());
  ExpectTopNullableHasValue(false);

  auto inner = MakeString(std::string{kInnerValue});
  auto nullable_with_value = MakeNullable(inner);
  PushObject(nullable_with_value);
  auto is_null = MakeSimple("IsNull");
  ASSERT_TRUE(is_null);
  EXPECT_TRUE(is_null->Execute(data_).has_value());
  EXPECT_FALSE(PopBool());

  // NullCoalesce with value keeps original
  PushInt(kCoalesceInt);
  PushObject(nullable_with_value);
  auto coalesce_keep = MakeSimple("NullCoalesce");
  ASSERT_TRUE(coalesce_keep);
  EXPECT_TRUE(coalesce_keep->Execute(data_).has_value());
  ExpectTopNullableHasValue(true);
  PopObject();

  // SafeCall: non-null returns int, wrapped into Nullable
  auto safe_func = MakeStubFunction(std::string{kSafeMethodName}, 1, [kSafeReturnValue](auto& data) {
    data.memory.machine_stack.emplace(static_cast<int64_t>(kSafeReturnValue));
    return ExecutionResult::kNormal;
  });
  ASSERT_TRUE(function_repo_.Add(std::move(safe_func)).has_value());

  auto nullable_val = MakeNullable(inner);
  PushObject(nullable_val);
  auto safe_call = MakeStringCmd("SafeCall", std::string{kSafeMethodName});
  ASSERT_TRUE(safe_call);
  EXPECT_TRUE(safe_call->Execute(data_).has_value());
  ExpectTopNullableHasValue(true);

  // SafeCall null branch keeps nullable and discards args
  auto nullable_null = MakeNullable(nullptr);
  PushObject(nullable_null);
  auto safe_call_null = MakeStringCmd("SafeCall", std::string{kSafeMethodName});
  ASSERT_TRUE(safe_call_null);
  EXPECT_TRUE(safe_call_null->Execute(data_).has_value());
  ExpectTopNullableHasValue(false);

  auto unwrap_obj = MakeNullable(inner);
  PushObject(unwrap_obj);
  auto unwrap = MakeSimple("Unwrap");
  ASSERT_TRUE(unwrap);
  EXPECT_TRUE(unwrap->Execute(data_).has_value());
  ASSERT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
  auto unwrapped = PopObject();
  auto* unwrapped_str = ovum::vm::runtime::GetDataPointer<std::string>(unwrapped);
  EXPECT_EQ(*unwrapped_str, kInnerValue);
}

TEST_F(BuiltinTestSuite, TypeOperations) {
  constexpr int64_t kValue = 5;
  constexpr std::string_view kTypeName = "int";

  PushInt(kValue);
  auto type_of = MakeSimple("TypeOf");
  ASSERT_TRUE(type_of);
  EXPECT_TRUE(type_of->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kTypeName});
  PopObject();

  PushInt(kValue);
  auto is_type = MakeStringCmd("IsType", std::string{kTypeName});
  ASSERT_TRUE(is_type);
  EXPECT_TRUE(is_type->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  auto size_of = MakeStringCmd("SizeOf", std::string{kTypeName});
  ASSERT_TRUE(size_of);
  EXPECT_TRUE(size_of->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), static_cast<int64_t>(sizeof(int64_t)));
}

TEST_F(BuiltinTestSuite, InputOutputCommands) {
  constexpr std::string_view kInputData = "line\nC 123 4.5";
  constexpr std::string_view kExpectedLine = "line";
  constexpr char kExpectedChar = 'C';
  constexpr int64_t kExpectedInt = 123;
  constexpr double kExpectedFloat = 4.5;
  constexpr std::string_view kPrintString = "out";
  constexpr std::string_view kPrintLineString = "line";
  constexpr std::string_view kCombinedOutput = "outline\n";

  input_stream_.str(std::string{kInputData});

  auto read_line = MakeSimple("ReadLine");
  ASSERT_TRUE(read_line);
  EXPECT_TRUE(read_line->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kExpectedLine});
  PopObject();

  auto read_char = MakeSimple("ReadChar");
  ASSERT_TRUE(read_char);
  EXPECT_TRUE(read_char->Execute(data_).has_value());
  EXPECT_EQ(PopChar(), kExpectedChar);

  auto read_int = MakeSimple("ReadInt");
  ASSERT_TRUE(read_int);
  EXPECT_TRUE(read_int->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kExpectedInt);

  auto read_float = MakeSimple("ReadFloat");
  ASSERT_TRUE(read_float);
  EXPECT_TRUE(read_float->Execute(data_).has_value());
  EXPECT_DOUBLE_EQ(PopDouble(), kExpectedFloat);

  auto to_print = MakeString(std::string{kPrintString});
  PushObject(to_print);
  auto print_cmd = MakeSimple("Print");
  ASSERT_TRUE(print_cmd);
  EXPECT_TRUE(print_cmd->Execute(data_).has_value());
  EXPECT_EQ(output_stream_.str(), std::string{kPrintString});

  auto to_print_line = MakeString(std::string{kPrintLineString});
  PushObject(to_print_line);
  auto print_line_cmd = MakeSimple("PrintLine");
  ASSERT_TRUE(print_line_cmd);
  EXPECT_TRUE(print_line_cmd->Execute(data_).has_value());
  EXPECT_EQ(output_stream_.str(), std::string{kCombinedOutput});
}

TEST_F(BuiltinTestSuite, TimeCommands) {
  constexpr int64_t kSleepMs = 1;
  constexpr int64_t kEpochSeconds = 0;
  constexpr std::string_view kYearFormat = "%Y";
  constexpr std::string_view kDateFormat = "%Y-%m-%d";
  constexpr std::string_view kEpochDate = "1970-01-01";
  constexpr int64_t kPositiveThreshold = 0;

  auto unix_time = MakeSimple("UnixTime");
  ASSERT_TRUE(unix_time);
  EXPECT_TRUE(unix_time->Execute(data_).has_value());
  auto first = PopInt();

  std::this_thread::sleep_for(std::chrono::milliseconds(kSleepMs));
  EXPECT_TRUE(unix_time->Execute(data_).has_value());
  auto second = PopInt();
  EXPECT_LE(first, second);

  auto ms_cmd = MakeSimple("UnixTimeMs");
  ASSERT_TRUE(ms_cmd);
  EXPECT_TRUE(ms_cmd->Execute(data_).has_value());
  auto ms = PopInt();
  EXPECT_GT(ms, kPositiveThreshold);

  auto ns_cmd = MakeSimple("UnixTimeNs");
  ASSERT_TRUE(ns_cmd);
  EXPECT_TRUE(ns_cmd->Execute(data_).has_value());
  auto ns = PopInt();
  EXPECT_GT(ns, kPositiveThreshold);

  auto nano = MakeSimple("NanoTime");
  ASSERT_TRUE(nano);
  EXPECT_TRUE(nano->Execute(data_).has_value());
  auto nano_val = PopInt();
  EXPECT_GT(nano_val, kPositiveThreshold);

  auto format_str = MakeString(std::string{kYearFormat});
  PushInt(kEpochSeconds);
  PushObject(format_str);
  auto format_cmd = MakeSimple("FormatDateTime");
  ASSERT_TRUE(format_cmd);
  EXPECT_TRUE(format_cmd->Execute(data_).has_value());
  {
    auto* str = ovum::vm::runtime::GetDataPointer<std::string>(std::get<void*>(memory_.machine_stack.top()));
    EXPECT_FALSE(str->empty());
    PopObject();
  }

  auto fmt = MakeString(std::string{kDateFormat});
  auto date = MakeString(std::string{kEpochDate});
  PushObject(date);
  PushObject(fmt);
  auto parse_cmd = MakeSimple("ParseDateTime");
  ASSERT_TRUE(parse_cmd);
  EXPECT_TRUE(parse_cmd->Execute(data_).has_value());
  EXPECT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
  auto int_obj = PopObject();
  auto* int_data = ovum::vm::runtime::GetDataPointer<int64_t>(int_obj);
  EXPECT_NE(int_data, nullptr);
}

TEST_F(BuiltinTestSuite, FileSystemCommands) {
  constexpr std::string_view kTempDirName = "ovum_vm_cmd_tests";
  constexpr std::string_view kFileName = "file.txt";
  constexpr std::string_view kCopyName = "copy.txt";
  constexpr std::string_view kMovedName = "moved.txt";
  constexpr std::string_view kFileContents = "abc";
  namespace fs = std::filesystem;
  const fs::path temp_dir = fs::temp_directory_path() / std::string{kTempDirName};
  auto dir_str = MakeString(temp_dir.string());
  PushObject(dir_str);
  auto create_dir = MakeSimple("CreateDirectory");
  ASSERT_TRUE(create_dir);
  EXPECT_TRUE(create_dir->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushObject(dir_str);
  auto dir_exists = MakeSimple("DirectoryExists");
  ASSERT_TRUE(dir_exists);
  EXPECT_TRUE(dir_exists->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  const fs::path file_path = temp_dir / std::string{kFileName};
  {
    std::ofstream ofs(file_path);
    ofs << kFileContents;
  }

  auto file_str = MakeString(file_path.string());
  PushObject(file_str);
  auto file_exists = MakeSimple("FileExists");
  ASSERT_TRUE(file_exists);
  EXPECT_TRUE(file_exists->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  const fs::path copy_path = temp_dir / std::string{kCopyName};
  auto copy_str = MakeString(copy_path.string());
  PushObject(copy_str);
  PushObject(file_str);
  auto copy_cmd = MakeSimple("CopyFile");
  ASSERT_TRUE(copy_cmd);
  EXPECT_TRUE(copy_cmd->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());
  EXPECT_TRUE(fs::exists(copy_path));

  PushObject(copy_str);
  auto delete_file = MakeSimple("DeleteFile");
  ASSERT_TRUE(delete_file);
  EXPECT_TRUE(delete_file->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  const std::filesystem::path moved_path = temp_dir / std::string{kMovedName};
  auto moved_str = MakeString(moved_path.string());
  // recreate file for move
  {
    std::ofstream ofs(file_path);
    ofs << kFileContents;
  }
  PushObject(moved_str);
  PushObject(file_str);
  auto move_cmd = MakeSimple("MoveFile");
  ASSERT_TRUE(move_cmd);
  EXPECT_TRUE(move_cmd->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());
  EXPECT_TRUE(std::filesystem::exists(moved_path));

  PushObject(dir_str);
  auto list_dir = MakeSimple("ListDirectory");
  ASSERT_TRUE(list_dir);
  EXPECT_TRUE(list_dir->Execute(data_).has_value());
  ASSERT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
  PopObject(); // discard list result

  PushObject(dir_str);
  auto delete_dir = MakeSimple("DeleteDirectory");
  ASSERT_TRUE(delete_dir);
  EXPECT_TRUE(delete_dir->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  auto cwd_cmd = MakeSimple("GetCurrentDirectory");
  ASSERT_TRUE(cwd_cmd);
  EXPECT_TRUE(cwd_cmd->Execute(data_).has_value());
  auto current_dir_obj = PopObject();
  PushObject(current_dir_obj);
  auto change_dir = MakeSimple("ChangeDirectory");
  ASSERT_TRUE(change_dir);
  EXPECT_TRUE(change_dir->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());
}

TEST_F(BuiltinTestSuite, SystemCommands) {
  constexpr std::string_view kEnvName = "OVUM_VM_TEST_ENV";
  constexpr std::string_view kEnvValue = "VALUE";
  constexpr int64_t kSleepMsValue = 1;
  constexpr int64_t kSleepNsValue = 1000;
  constexpr int64_t kPositivePidThreshold = 0;
  constexpr int64_t kMinProcessorCount = 1;

  auto pid_cmd = MakeSimple("GetProcessId");
  ASSERT_TRUE(pid_cmd);
  EXPECT_TRUE(pid_cmd->Execute(data_).has_value());
  EXPECT_GT(PopInt(), kPositivePidThreshold);

  auto env_name = MakeString(std::string{kEnvName});
  auto env_value = MakeString(std::string{kEnvValue});
  PushObject(env_value);
  PushObject(env_name);
  auto set_env = MakeSimple("SetEnvironmentVar");
  ASSERT_TRUE(set_env);
  EXPECT_TRUE(set_env->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  PushObject(env_name);
  auto get_env = MakeSimple("GetEnvironmentVar");
  ASSERT_TRUE(get_env);
  EXPECT_TRUE(get_env->Execute(data_).has_value());
  {
    ASSERT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
    auto* nullable_ptr = GetDataPointer<void*>(std::get<void*>(memory_.machine_stack.top()));
    if (*nullable_ptr != nullptr) {
      auto* str_ptr = GetDataPointer<std::string>(*nullable_ptr);
      EXPECT_EQ(*str_ptr, kEnvValue);
    }
    PopObject();
  }

  auto sleep_ms = MakeSimple("SleepMs");
  ASSERT_TRUE(sleep_ms);
  PushInt(kSleepMsValue);
  EXPECT_TRUE(sleep_ms->Execute(data_).has_value());

  auto sleep_ns = MakeSimple("SleepNs");
  ASSERT_TRUE(sleep_ns);
  PushInt(kSleepNsValue);
  EXPECT_TRUE(sleep_ns->Execute(data_).has_value());

  auto cpu_count = MakeSimple("GetProcessorCount");
  ASSERT_TRUE(cpu_count);
  EXPECT_TRUE(cpu_count->Execute(data_).has_value());
  EXPECT_GE(PopInt(), kMinProcessorCount);
}

TEST_F(BuiltinTestSuite, RandomCommands) {
  constexpr int64_t kSeedValue = 12345;
  constexpr int64_t kRangeMin = 1;
  constexpr int64_t kRangeMax = 3;
  constexpr double kFloatRangeMin = 0.5;
  constexpr double kFloatRangeMax = 0.6;
  constexpr double kMinUnit = 0.0;
  constexpr double kMaxUnit = 1.0;

  auto seed = MakeSimple("SeedRandom");
  ASSERT_TRUE(seed);
  PushInt(kSeedValue);
  EXPECT_TRUE(seed->Execute(data_).has_value());

  auto rand_range = MakeSimple("RandomRange");
  PushInt(kRangeMin);
  PushInt(kRangeMax);
  ASSERT_TRUE(rand_range);
  EXPECT_TRUE(rand_range->Execute(data_).has_value());
  auto first = PopInt();
  EXPECT_GE(first, kRangeMin);
  EXPECT_LE(first, kRangeMax);

  PushInt(kSeedValue);
  EXPECT_TRUE(seed->Execute(data_).has_value());
  PushInt(kRangeMin);
  PushInt(kRangeMax);
  EXPECT_TRUE(rand_range->Execute(data_).has_value());
  auto second = PopInt();
  EXPECT_EQ(first, second); // deterministic after seeding

  auto rand_float_range = MakeSimple("RandomFloatRange");
  PushFloat(kFloatRangeMin);
  PushFloat(kFloatRangeMax);
  ASSERT_TRUE(rand_float_range);
  EXPECT_TRUE(rand_float_range->Execute(data_).has_value());
  auto val = PopDouble();
  EXPECT_GE(val, kFloatRangeMin);
  EXPECT_LE(val, kFloatRangeMax);

  auto rand_cmd = MakeSimple("Random");
  ASSERT_TRUE(rand_cmd);
  EXPECT_TRUE(rand_cmd->Execute(data_).has_value());
  PopInt();

  auto rand_float = MakeSimple("RandomFloat");
  ASSERT_TRUE(rand_float);
  EXPECT_TRUE(rand_float->Execute(data_).has_value());
  auto rf = PopDouble();
  EXPECT_GE(rf, kMinUnit);
  EXPECT_LE(rf, kMaxUnit);
}

TEST_F(BuiltinTestSuite, MemoryAndOsInfoCommands) {
  constexpr int64_t kZero = 0;

  auto mem_cmd = MakeSimple("GetMemoryUsage");
  ASSERT_TRUE(mem_cmd);
  EXPECT_TRUE(mem_cmd->Execute(data_).has_value());
  auto usage = PopInt();
  EXPECT_GE(usage, kZero);

  auto peak_cmd = MakeSimple("GetPeakMemoryUsage");
  ASSERT_TRUE(peak_cmd);
  EXPECT_TRUE(peak_cmd->Execute(data_).has_value());
  EXPECT_GE(PopInt(), usage);

  auto gc_cmd = MakeSimple("ForceGarbageCollection");
  ASSERT_TRUE(gc_cmd);
  EXPECT_TRUE(gc_cmd->Execute(data_).has_value());

  auto os_name = MakeSimple("GetOsName");
  ASSERT_TRUE(os_name);
  EXPECT_TRUE(os_name->Execute(data_).has_value());
  {
    ASSERT_TRUE(std::holds_alternative<void*>(memory_.machine_stack.top()));
    auto* str = GetDataPointer<std::string>(std::get<void*>(memory_.machine_stack.top()));
    EXPECT_FALSE(str->empty());
    PopObject();
  }

  auto os_ver = MakeSimple("GetOsVersion");
  ASSERT_TRUE(os_ver);
  EXPECT_TRUE(os_ver->Execute(data_).has_value());
  PopObject();

  auto arch = MakeSimple("GetArchitecture");
  ASSERT_TRUE(arch);
  EXPECT_TRUE(arch->Execute(data_).has_value());
  PopObject();

  auto user = MakeSimple("GetUserName");
  ASSERT_TRUE(user);
  EXPECT_TRUE(user->Execute(data_).has_value());
  PopObject();

  auto home = MakeSimple("GetHomeDirectory");
  ASSERT_TRUE(home);
  EXPECT_TRUE(home->Execute(data_).has_value());
  PopObject();
}

TEST_F(BuiltinTestSuite, TypeIntrospectionAndSizeOf) {
  constexpr uint8_t kByteValue = 1;
  constexpr std::string_view kByteType = "byte";
  constexpr std::string_view kStringValue = "text";
  constexpr std::string_view kStringType = "String";
  constexpr auto kStringSize = static_cast<int64_t>(sizeof(ovum::vm::runtime::ObjectDescriptor) + sizeof(std::string));

  PushByte(kByteValue);
  auto type_of = MakeSimple("TypeOf");
  ASSERT_TRUE(type_of);
  EXPECT_TRUE(type_of->Execute(data_).has_value());
  ExpectTopStringEquals(std::string{kByteType});
  PopObject();

  auto obj = MakeString(std::string{kStringValue});
  PushObject(obj);
  auto is_type = MakeStringCmd("IsType", std::string{kStringType});
  ASSERT_TRUE(is_type);
  EXPECT_TRUE(is_type->Execute(data_).has_value());
  EXPECT_TRUE(PopBool());

  auto size_obj = MakeStringCmd("SizeOf", std::string{kStringType});
  ASSERT_TRUE(size_obj);
  EXPECT_TRUE(size_obj->Execute(data_).has_value());
  EXPECT_EQ(PopInt(), kStringSize);
}

TEST_F(BuiltinTestSuite, InteropCommandNegativePath) {
  constexpr std::string_view kMissingLib = "nonexistent_library.so";
  constexpr std::string_view kMissingFunc = "missing";
  constexpr uint8_t kOutputInit = 0;
  const std::vector<uint8_t> empty_bytes{};
  const std::vector<uint8_t> output_bytes{kOutputInit};

  auto lib = MakeString(std::string{kMissingLib});
  auto func = MakeString(std::string{kMissingFunc});
  auto in_array = MakeByteArray(empty_bytes);
  auto out_array = MakeByteArray(output_bytes);
  PushObject(out_array);
  PushObject(in_array);
  PushObject(func);
  PushObject(lib);
  auto interop = MakeSimple("Interop");
  ASSERT_TRUE(interop);
  EXPECT_FALSE(interop->Execute(data_).has_value());
}

TEST_F(BuiltinTestSuite, ExitCommandTerminates) {
  constexpr int64_t kExitCode = 0;
  auto exit_cmd = MakeSimple("Exit");
  PushInt(kExitCode);
  ASSERT_TRUE(exit_cmd);

#ifndef WITH_VALGRIND // Known issue with EXPECT_EXIT in combination with Valgrind
  EXPECT_EXIT(ASSERT_FALSE(exit_cmd->Execute(data_).has_value()), ::testing::ExitedWithCode(kExitCode), "");
#endif
}

TEST_F(BuiltinTestSuite, CompoundScenario) {
  // Push numbers, add, to string, print
  constexpr int64_t kFirstValue = 5;
  constexpr int64_t kSecondValue = 7;
  constexpr std::string_view kResultStr = "12\n";

  auto push_int = MakeIntCmd("PushInt", kFirstValue);
  ASSERT_TRUE(push_int);
  EXPECT_TRUE(push_int->Execute(data_).has_value());
  auto push_int2 = MakeIntCmd("PushInt", kSecondValue);
  ASSERT_TRUE(push_int2);
  EXPECT_TRUE(push_int2->Execute(data_).has_value());

  auto add = MakeSimple("IntAdd");
  ASSERT_TRUE(add);
  EXPECT_TRUE(add->Execute(data_).has_value());
  auto to_str = MakeSimple("IntToString");
  ASSERT_TRUE(to_str);
  EXPECT_TRUE(to_str->Execute(data_).has_value());
  auto print_line = MakeSimple("PrintLine");
  ASSERT_TRUE(print_line);
  EXPECT_TRUE(print_line->Execute(data_).has_value());
  EXPECT_FALSE(output_stream_.str().empty());
  EXPECT_EQ(output_stream_.str(), std::string{kResultStr});
}
