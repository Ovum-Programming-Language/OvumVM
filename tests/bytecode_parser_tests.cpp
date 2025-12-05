#include "test_suites/BytecodeParserTestSuite.hpp"

// ============================================================================
// InitStaticParser Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, InitStatic_EmptyBlock) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_SingleCommand) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_MultipleCommands) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushInt 1 PushInt 2 IntAdd Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_WithNestedIf) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { if { PushBool true } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_WithNestedWhile) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { while { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_AllCommandTypes) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { PushString "hello" PushChar "a" PushInt 42 PushFloat 3.14 PushBool true NewArray arrName Call funcName Return })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, InitStatic_MissingKeyword_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("{ Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Unknown top-level declaration");
}

TEST_F(BytecodeParserTestSuite, InitStatic_MissingOpeningBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, InitStatic_MissingClosingBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { Return");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "");
}

TEST_F(BytecodeParserTestSuite, InitStatic_MultipleBlocks_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { Return } init-static { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Multiple init-static blocks");
}

// ============================================================================
// FunctionParser Tests - Regular Functions
// ============================================================================

TEST_F(BytecodeParserTestSuite, Function_Regular_Basic) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  ASSERT_EQ(result.value(), nullptr) << "Init-static block should be null for function-only parse";
  AssertFunctionExists(func_repo, "funcName", 0);
  AssertFunctionCount(func_repo, 1);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_Arity0) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 func0 { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "func0", 0);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_Arity1) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:1 func1 { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "func1", 1);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_ArityTen) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:10 funcTen { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcTen", kJitBoundary);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_WithCommands) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 funcWithCommands { PushInt 42 Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcWithCommands", 0);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_WithNestedIf) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 funcIf { if { PushBool true } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcIf", 0);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_WithNestedWhile) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 funcWhile { while { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcWhile", 0);
}

TEST_F(BytecodeParserTestSuite, Function_Regular_WithJit) {
  auto parser = CreateParserWithJit(kJitBoundary);
  auto tokens = TokenizeString("function:0 funcJit { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  auto func_result = func_repo.GetByName("funcJit");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "JitFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Regular_WithoutJit) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("function:0 funcRegular { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  auto func_result = func_repo.GetByName("funcRegular");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "RegularFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Regular_NoJit_WithJitFactory) {
  auto parser = CreateParserWithJit(kJitBoundary);
  auto tokens = TokenizeString("no-jit function:0 funcNoJit { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  auto func_result = func_repo.GetByName("funcNoJit");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "RegularFunction");
}

// ============================================================================
// FunctionParser Tests - Pure Functions
// ============================================================================

TEST_F(BytecodeParserTestSuite, Function_Pure_WithoutTypes) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("pure() function:0 funcPure { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcPure", 0);
  auto func_result = func_repo.GetByName("funcPure");
  ASSERT_TRUE(func_result.has_value());
  // Pure with empty types should create RegularFunction (not PureFunction)
  AssertFunctionType(func_result.value(), "RegularFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Pure_WithSingleType) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("pure(Type1) function:1 funcPure1 { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcPure1", 1);
  auto func_result = func_repo.GetByName("funcPure1");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "PureFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Pure_WithMultipleTypes) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("pure(Type1,Type2,Type3) function:3 funcPure3 { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcPure3", 3);
  auto func_result = func_repo.GetByName("funcPure3");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "PureFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Pure_WithJit) {
  auto parser = CreateParserWithJit(kJitBoundary);
  auto tokens = TokenizeString("pure(Type1) function:1 funcPureJit { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  auto func_result = func_repo.GetByName("funcPureJit");
  ASSERT_TRUE(func_result.has_value());
  AssertFunctionType(func_result.value(), "PureJitFunction");
}

TEST_F(BytecodeParserTestSuite, Function_Pure_NoJit_WithJitFactory) {
  auto parser = CreateParserWithJit(kJitBoundary);
  auto tokens = TokenizeString("pure(Object) no-jit function:1 funcPureNoJit { Return }");

  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcPureNoJit", 1);
}

TEST_F(BytecodeParserTestSuite, Function_Pure_WithCommands) {
  auto parser = CreateParserWithoutJit();
  auto tokens = TokenizeString("pure(Type1) function:1 funcPureCmd { PushInt 42 Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionExists(func_repo, "funcPureCmd", 1);
}

// ============================================================================
// FunctionParser Tests - Negative Cases
// ============================================================================

TEST_F(BytecodeParserTestSuite, Function_MissingKeyword_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(":0 funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Unknown top-level declaration");
}

TEST_F(BytecodeParserTestSuite, Function_MissingColon_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function 0 funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected ':'");
}

TEST_F(BytecodeParserTestSuite, Function_MissingArity_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function: funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected integer literal");
}

TEST_F(BytecodeParserTestSuite, Function_MissingName_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function:0 { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected identifier");
}

TEST_F(BytecodeParserTestSuite, Function_MissingOpeningBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function:0 funcName }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, Function_MissingClosingBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function:0 funcName { Return");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "");
}

TEST_F(BytecodeParserTestSuite, Function_InvalidPureSyntax_MissingParen_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("pure function:0 funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '('");
}

TEST_F(BytecodeParserTestSuite, Function_DuplicateName_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function:0 funcName { } function:0 funcName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Failed to add function");
}

// ============================================================================
// VtableParser Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, Vtable_Minimal_AutoDestructor) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
  AssertVtableCount(vtable_repo, 1);
  // Should auto-generate destructor
  AssertFunctionExists(func_repo, "ClassName_destructor_<M>", 1);
  AssertFunctionCount(func_repo, 1);
}

TEST_F(BytecodeParserTestSuite, Vtable_WithSize) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { size: 100 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
  auto vtable_result = vtable_repo.GetByName("ClassName");
  ASSERT_TRUE(vtable_result.has_value());
  // Note: VirtualTable doesn't expose size directly, but we can verify it exists
}

TEST_F(BytecodeParserTestSuite, Vtable_WithInterfaces) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { interfaces { I1, I2 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_WithMethods) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { methods { virt1:real1, virt2:real2 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_WithVartable) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { vartable { field1:int@0, field2:float@8 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_WithAllSections) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(vtable ClassName { size: 100 interfaces { I1, I2 } methods { virt1:real1 } vartable { field1:int@0 } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_WithManualDestructor) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(R"(vtable ClassName { methods { ClassName_destructor_<M>:ClassName_destructor_<M> } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
  // Should not auto-generate destructor since one is provided
  AssertFunctionCount(func_repo, 0);
}

TEST_F(BytecodeParserTestSuite, Vtable_MultipleInterfaces) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { interfaces { I1, I2, I3, I4 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_MultipleMethods) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { methods { v1:r1, v2:r2, v3:r3, v4:r4 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableExists(vtable_repo, "ClassName");
}

TEST_F(BytecodeParserTestSuite, Vtable_MissingKeyword_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("ClassName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Unknown top-level declaration");
}

TEST_F(BytecodeParserTestSuite, Vtable_MissingName_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected identifier");
}

TEST_F(BytecodeParserTestSuite, Vtable_MissingOpeningBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, Vtable_InvalidSizeSyntax_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { size 100 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected ':'");
}

TEST_F(BytecodeParserTestSuite, Vtable_InvalidInterfacesSyntax_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { interfaces I1 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, Vtable_InvalidMethodsSyntax_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { methods virt1:real1 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, Vtable_InvalidVartableSyntax_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { vartable { field1:type1 } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '@'");
}

TEST_F(BytecodeParserTestSuite, Vtable_UnknownDirective_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { unknown { } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Unknown vtable directive");
}

TEST_F(BytecodeParserTestSuite, Vtable_DuplicateName_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable ClassName { } vtable ClassName { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Failed to add vtable");
}

// ============================================================================
// IfParser Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, If_Simple) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { if { PushBool true } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(ParseSuccessfully(parser, tokens, func_repo, vtable_repo).value());
}

TEST_F(BytecodeParserTestSuite, If_Else) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { if { PushBool true } then { Return } else { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, If_ElseIf) {
  auto parser = CreateParserWithJit();
  auto tokens =
      TokenizeString("init-static { if { PushBool true } then { Return } else if { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, If_ElseIfElse) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { if { PushBool true } then { Return } else if { PushBool false } then { Return } else { Return } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, If_MultipleElseIf) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { if { PushBool true } then { Return } else if { PushBool false } then { Return } else if { PushBool true } then { Return } else { Return } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, If_Nested) {
  auto parser = CreateParserWithJit();
  auto tokens =
      TokenizeString(R"(init-static { if { PushBool true } then { if { PushBool false } then { Return } } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, If_MissingKeyword_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { { PushBool true } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Command expected");
}

TEST_F(BytecodeParserTestSuite, If_MissingThen_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { if { PushBool true } { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected keyword 'then'");
}

TEST_F(BytecodeParserTestSuite, If_MissingConditionBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { if PushBool true } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

TEST_F(BytecodeParserTestSuite, If_InvalidElseIf_Error) {
  auto parser = CreateParserWithJit();
  auto tokens =
      TokenizeString("init-static { if { PushBool true } then { Return } else { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "");
}

// ============================================================================
// WhileParser Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, While_Simple) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { while { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, While_WithCommands) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { while { PushInt 1 PushInt 2 IntAdd } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, While_Nested) {
  auto parser = CreateParserWithJit();
  auto tokens =
      TokenizeString(R"(init-static { while { PushBool true } then { while { PushBool false } then { Return } } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, While_MissingKeyword_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { { PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Command expected");
}

TEST_F(BytecodeParserTestSuite, While_MissingThen_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { while { PushBool false } { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected keyword 'then'");
}

TEST_F(BytecodeParserTestSuite, While_MissingConditionBrace_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { while PushBool false } then { Return } }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected '{'");
}

// ============================================================================
// CommandParser Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, Command_String_PushString) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(R"(init-static { PushString "hello" })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_String_PushChar) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(R"(init-static { PushChar "a" })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Integer_PushInt) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushInt 42 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Integer_PushByte) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushByte 5 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Integer_LoadLocal) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { LoadLocal 0 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Integer_SetLocal) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { SetLocal 1 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Float_PushFloat) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushFloat 3.14 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Boolean_PushBool_True) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushBool true }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Boolean_PushBool_False) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushBool false }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Identifier_NewArray) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { NewArray arrName }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Identifier_Call) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { Call funcName }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Identifier_CallVirtual) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { CallVirtual virtName }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Simple_Return) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_Simple_IntAdd) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { IntAdd }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}

TEST_F(BytecodeParserTestSuite, Command_MissingStringArg_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushString }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected string literal");
}

TEST_F(BytecodeParserTestSuite, Command_MissingIntArg_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushInt }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected integer literal");
}

TEST_F(BytecodeParserTestSuite, Command_WrongArgType_Error) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("init-static { PushString 42 }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  AssertParseError(parser, tokens, func_repo, vtable_repo, "Expected string literal");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(BytecodeParserTestSuite, Integration_MultipleFunctions) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("function:0 func1 { Return } function:1 func2 { Return } function:2 func3 { Return }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionCount(func_repo, 3);
  AssertFunctionExists(func_repo, "func1", 0);
  AssertFunctionExists(func_repo, "func2", 1);
  AssertFunctionExists(func_repo, "func3", 2);
}

TEST_F(BytecodeParserTestSuite, Integration_MultipleVtables) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("vtable Class1 { } vtable Class2 { } vtable Class3 { }");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertVtableCount(vtable_repo, 3);
  AssertVtableExists(vtable_repo, "Class1");
  AssertVtableExists(vtable_repo, "Class2");
  AssertVtableExists(vtable_repo, "Class3");
}

TEST_F(BytecodeParserTestSuite, Integration_InitStaticWithFunctionsAndVtables) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { Return } function:0 func1 { Return } vtable Class1 { } function:1 func2 { Return })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 2 + vtable_repo.GetCount()); // destructor for Class1
  AssertVtableCount(vtable_repo, 1);
}

TEST_F(BytecodeParserTestSuite, Integration_DifferentJitPureCombinations) {
  auto parser = CreateParserWithJit(kJitBoundary);
  auto tokens = TokenizeString(
      R"(function:0 regular { Return } function:0 jitFunc { Return } pure(Type1) function:1 pureFunc { Return } no-jit function:0 noJitFunc { Return })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertFunctionCount(func_repo, 4);

  auto regular_result = func_repo.GetByName("regular");
  ASSERT_TRUE(regular_result.has_value());
  AssertFunctionType(regular_result.value(), "JitFunction");

  auto jit_result = func_repo.GetByName("jitFunc");
  ASSERT_TRUE(jit_result.has_value());
  AssertFunctionType(jit_result.value(), "JitFunction");

  auto pure_result = func_repo.GetByName("pureFunc");
  ASSERT_TRUE(pure_result.has_value());
  AssertFunctionType(pure_result.value(), "PureJitFunction");

  auto no_jit_result = func_repo.GetByName("noJitFunc");
  ASSERT_TRUE(no_jit_result.has_value());
  AssertFunctionType(no_jit_result.value(), "RegularFunction");
}

TEST_F(BytecodeParserTestSuite, Integration_AllDeclarationTypes) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { Return } function:0 func1 { Return } vtable Class1 { } function:1 func2 { Return } vtable Class2 { size: 100 })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  AssertInitStaticBlockExists(result.value());
  AssertFunctionCount(func_repo, 2 + vtable_repo.GetCount()); // destructors for Class1 and Class2
  AssertVtableCount(vtable_repo, 2);
}

TEST_F(BytecodeParserTestSuite, Integration_EmptyInput) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString("");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
  // Empty input should result in null init-static block
  ASSERT_EQ(result.value(), nullptr);
  AssertFunctionCount(func_repo, 0);
  AssertVtableCount(vtable_repo, 0);
}

TEST_F(BytecodeParserTestSuite, Integration_ComplexNestedStructures) {
  auto parser = CreateParserWithJit();
  auto tokens = TokenizeString(
      R"(init-static { if { PushBool true } then { while { PushBool false } then { if { PushBool true } then { Return } else { Return } } } else { Return } })");
  ovum::vm::execution_tree::FunctionRepository func_repo;
  ovum::vm::runtime::VirtualTableRepository vtable_repo;

  auto parsing_result = ParseSuccessfully(parser, tokens, func_repo, vtable_repo);
}
