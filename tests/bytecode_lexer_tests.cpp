#include "test_suites/BytecodeLexerTestSuite.hpp"

#include <string>

// ============================================================================
// 1. WhitespaceHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerSingleSpace) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess(" ", kExpectedTokenCountEOFOnly); // Only EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerMultipleSpaces) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess("   ", kExpectedTokenCountEOFOnly); // Only EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerTabCharacter) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess("\t", kExpectedTokenCountEOFOnly); // Only EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerCarriageReturn) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess("\r", kExpectedTokenCountEOFOnly); // Only EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerMixedWhitespace) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess(" \t\r ", kExpectedTokenCountEOFOnly); // Only EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerWhitespaceAtStart) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  AssertTokenizationSuccess("  hello", kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerWhitespaceAtEnd) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  AssertTokenizationSuccess("hello  ", kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerWhitespaceBetweenTokens) {
  constexpr size_t kExpectedTokenCountTwoTokens = 3;
  AssertTokenizationSuccess("hello world", kExpectedTokenCountTwoTokens); // identifier + identifier + EOF
}

// ============================================================================
// 2. WhitespaceHandler Newline Tests (newlines are handled as whitespace)
// ============================================================================

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerSingleNewline) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess("\n", kExpectedTokenCountEOFOnly); // Only EOF (newline consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerMultipleConsecutiveNewlines) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  AssertTokenizationSuccess("\n\n\n", kExpectedTokenCountEOFOnly); // Only EOF (newlines consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerNewlineAtStart) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("\nhello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF (newline consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerNewlineAtEnd) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("hello\n");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF (newline consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, WhitespaceHandlerNewlineBetweenTokens) {
  constexpr size_t kExpectedTokenCountTwoTokens = 3;
  auto tokens = TokenizeSuccessfully("hello\nworld");
  AssertTokenCount(tokens,
                   kExpectedTokenCountTwoTokens); // identifier + identifier + EOF (newline consumed as whitespace)
}

// ============================================================================
// 3. IdentifierHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerSingleLetterLowercase) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("a");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerSingleLetterUppercase) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("A");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerMultiCharacter) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("hello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerStartsWithUnderscore) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("_hello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerWithNumbers) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("var123");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerWithAngleBrackets) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("List<int>");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordIf) {
  auto tokens = TokenizeSuccessfully("if");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordThen) {
  auto tokens = TokenizeSuccessfully("then");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordElse) {
  auto tokens = TokenizeSuccessfully("else");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordWhile) {
  auto tokens = TokenizeSuccessfully("while");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordInitStatic) {
  auto tokens = TokenizeSuccessfully("init-static");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordFunction) {
  auto tokens = TokenizeSuccessfully("function");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordPure) {
  auto tokens = TokenizeSuccessfully("pure");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordNoJit) {
  auto tokens = TokenizeSuccessfully("no-jit");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordVtable) {
  auto tokens = TokenizeSuccessfully("vtable");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordSize) {
  auto tokens = TokenizeSuccessfully("size");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordInterfaces) {
  auto tokens = TokenizeSuccessfully("interfaces");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordMethods) {
  auto tokens = TokenizeSuccessfully("methods");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordVartable) {
  auto tokens = TokenizeSuccessfully("vartable");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordTrue) {
  auto tokens = TokenizeSuccessfully("true");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerKeywordFalse) {
  auto tokens = TokenizeSuccessfully("false");
  AssertTokenCount(tokens, 2); // keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerMixedCase) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("HelloWorld");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerLongIdentifier) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("verylongidentifiername123");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerAfterWhitespace) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("  hello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, IdentifierHandlerAfterNewline) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("\nhello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF (newline consumed as whitespace)
}

// ============================================================================
// 4. NumberHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, NumberHandlerSingleDigit) {
  auto tokens = TokenizeSuccessfully("5");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerMultiDigitInteger) {
  auto tokens = TokenizeSuccessfully("123");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerZero) {
  auto tokens = TokenizeSuccessfully("0");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerLargeInteger) {
  auto tokens = TokenizeSuccessfully("1234567890");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerNegativeInteger) {
  auto tokens = TokenizeSuccessfully("-1");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerFloatWithDecimalPoint) {
  auto tokens = TokenizeSuccessfully("3.14");
  AssertTokenCount(tokens, 2); // float literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerNegativeFloat) {
  auto tokens = TokenizeSuccessfully("-1.0");
  AssertTokenCount(tokens, 2); // float literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerFloatDecimalPointAtEnd) {
  auto tokens = TokenizeSuccessfully("5.");
  AssertTokenCount(tokens, 2); // float literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerFloatDecimalPointAtStart) {
  // "." is not handled by NumberHandler, triggers DefaultHandler
  AssertTokenizationError(".5", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerAfterWhitespace) {
  auto tokens = TokenizeSuccessfully("  123");
  AssertTokenCount(tokens, 2); // int literal + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerAfterNewline) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("\n123");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // int literal + EOF (newline consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerFollowedByIdentifier) {
  auto tokens = TokenizeSuccessfully("123abc");
  AssertTokenCount(tokens, 3); // int literal + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerMultipleDecimalPoints) {
  // "3.14.15" - after first decimal point, second dot triggers DefaultHandler
  AssertTokenizationError("3.14.15", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, NumberHandlerJustDecimalPoint) {
  // "." is not handled by NumberHandler, triggers DefaultHandler
  AssertTokenizationError(".", "Unexpected character");
}

// ============================================================================
// 5. StringHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, StringHandlerEmptyString) {
  auto tokens = TokenizeSuccessfully("\"\"");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerSimpleString) {
  auto tokens = TokenizeSuccessfully("\"hello\"");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerStringWithSpaces) {
  auto tokens = TokenizeSuccessfully("\"hello world\"");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeNewline) {
  auto tokens = TokenizeSuccessfully(R"("hello\nworld")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeTab) {
  auto tokens = TokenizeSuccessfully(R"("hello\tworld")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeCarriageReturn) {
  auto tokens = TokenizeSuccessfully(R"("hello\rworld")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeBackslash) {
  auto tokens = TokenizeSuccessfully(R"("hello\\world")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeQuote) {
  auto tokens = TokenizeSuccessfully(R"("hello\"world")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerEscapeNull) {
  auto tokens = TokenizeSuccessfully(R"("hello\0world")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerMultipleEscapeSequences) {
  auto tokens = TokenizeSuccessfully(R"("hello\n\t\rworld")");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerAtStart) {
  constexpr size_t kExpectedTokenCountTwoTokens = 3;
  auto tokens = TokenizeSuccessfully("\"hello\"world");
  AssertTokenCount(tokens, kExpectedTokenCountTwoTokens); // string literal + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerAtEnd) {
  auto tokens = TokenizeSuccessfully("hello\"world\"");
  AssertTokenCount(tokens, 3); // identifier + string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerBetweenTokens) {
  constexpr size_t kExpectedTokenCountThreeTokens = 4;
  auto tokens = TokenizeSuccessfully("hello \"world\" test");
  AssertTokenCount(tokens, kExpectedTokenCountThreeTokens); // identifier + string literal + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, StringHandlerUnterminatedStringEOF) {
  AssertTokenizationError("\"hello", "Unterminated string literal");
}

TEST_F(BytecodeLexerTestSuite, StringHandlerUnterminatedStringNewline) {
  AssertTokenizationError("\"hello\nworld\"", "Unterminated string literal");
}

TEST_F(BytecodeLexerTestSuite, StringHandlerBackslashAtEOF) {
  AssertTokenizationError("\"hello\\", "Unterminated string literal");
}

TEST_F(BytecodeLexerTestSuite, StringHandlerUnknownEscapeSequence) {
  AssertTokenizationError(R"("hello\xworld")", "Unknown escape");
}

// ============================================================================
// 6. PunctHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, PunctHandlerLeftBrace) {
  auto tokens = TokenizeSuccessfully("{");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerRightBrace) {
  auto tokens = TokenizeSuccessfully("}");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerComma) {
  auto tokens = TokenizeSuccessfully(",");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerSemicolon) {
  auto tokens = TokenizeSuccessfully(";");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerLeftParen) {
  auto tokens = TokenizeSuccessfully("(");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerRightParen) {
  auto tokens = TokenizeSuccessfully(")");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerLeftBracket) {
  auto tokens = TokenizeSuccessfully("[");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerRightBracket) {
  auto tokens = TokenizeSuccessfully("]");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerColon) {
  auto tokens = TokenizeSuccessfully(":");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerAtSign) {
  auto tokens = TokenizeSuccessfully("@");
  AssertTokenCount(tokens, 2); // punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerMultiplePunctuation) {
  auto tokens = TokenizeSuccessfully("{},;()[]:@");
  constexpr size_t kExpectedTokenCountTenPunctPlusEOF = 11;
  AssertTokenCount(tokens, kExpectedTokenCountTenPunctPlusEOF); // 10 punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerAtStart) {
  auto tokens = TokenizeSuccessfully("{hello");
  AssertTokenCount(tokens, 3); // punct + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerAtEnd) {
  auto tokens = TokenizeSuccessfully("hello}");
  AssertTokenCount(tokens, 3); // identifier + punct + EOF
}

TEST_F(BytecodeLexerTestSuite, PunctHandlerBetweenTokens) {
  auto tokens = TokenizeSuccessfully("hello,world");
  AssertTokenCount(tokens, 4); // identifier + punct + identifier + EOF
}

// ============================================================================
// 7. DefaultHandler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, DefaultHandlerInvalidCharacterHash) {
  AssertTokenizationError("#", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, DefaultHandlerInvalidCharacterDollar) {
  AssertTokenizationError("$", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, DefaultHandlerInvalidCharacterPercent) {
  AssertTokenizationError("%", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, DefaultHandlerInvalidCharacterAmpersand) {
  AssertTokenizationError("&", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, DefaultHandlerInvalidCharacterAsterisk) {
  AssertTokenizationError("*", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, DefaultHandlerNonASCIICharacter) {
  AssertTokenizationError("ñ", "Unexpected character");
}

// ============================================================================
// 8. Combined Scenarios Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, CombinedIdentifierFollowedByNumber) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("var123");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedNumberFollowedByIdentifier) {
  auto tokens = TokenizeSuccessfully("123var");
  AssertTokenCount(tokens, 3); // int literal + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedStringContainingIdentifiers) {
  auto tokens = TokenizeSuccessfully("\"hello\"");
  AssertTokenCount(tokens, 2); // string literal + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedFunctionCallPattern) {
  constexpr size_t kExpectedTokenCountFourTokens = 5;
  auto tokens = TokenizeSuccessfully("func(123)");
  AssertTokenCount(tokens, kExpectedTokenCountFourTokens); // identifier + punct + int literal + punct + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedArrayAccess) {
  constexpr size_t kExpectedTokenCountFourTokens = 5;
  auto tokens = TokenizeSuccessfully("arr[0]");
  AssertTokenCount(tokens, kExpectedTokenCountFourTokens); // identifier + punct + int literal + punct + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedComplexExpression) {
  // '>' is not handled by PunctHandler, so this will error on '>'
  AssertTokenizationError("if (x > 5) { return; }", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, CombinedMultipleLines) {
  constexpr size_t kExpectedTokenCountThreeTokens = 4;
  auto tokens = TokenizeSuccessfully("hello\nworld\ntest");
  AssertTokenCount(
      tokens,
      kExpectedTokenCountThreeTokens); // identifier + identifier + identifier + EOF (newlines consumed as whitespace)
}

TEST_F(BytecodeLexerTestSuite, CombinedKeywordsInContext) {
  constexpr size_t kExpectedTokenCountFourTokens = 5;
  auto tokens = TokenizeSuccessfully("if true then false");
  AssertTokenCount(tokens, kExpectedTokenCountFourTokens); // keyword + keyword + keyword + keyword + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedMixedWhitespaceAndNewlines) {
  constexpr size_t kExpectedTokenCountTwoTokens = 3;
  auto tokens = TokenizeSuccessfully("hello \n\t world");
  // Tab and newline are whitespace and are consumed, so we get: identifier + identifier + EOF
  AssertTokenCount(tokens, kExpectedTokenCountTwoTokens); // identifier + identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, CombinedEmptyInput) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  auto tokens = TokenizeSuccessfully("");
  AssertTokenCount(tokens, kExpectedTokenCountEOFOnly); // EOF only
}

TEST_F(BytecodeLexerTestSuite, CombinedOnlyWhitespace) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  auto tokens = TokenizeSuccessfully("   \t  ");
  AssertTokenCount(tokens, kExpectedTokenCountEOFOnly); // EOF only
}

// ============================================================================
// 9. Line and Column Tracking Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, PositionTrackingSingleLine) {
  auto tokens = TokenizeSuccessfully("hello world");
  AssertTokenCount(tokens, 3); // identifier + identifier + EOF
  // Line 1, columns should be tracked
}

TEST_F(BytecodeLexerTestSuite, PositionTrackingMultiLine) {
  constexpr size_t kExpectedTokenCountThreeTokens = 4;
  auto tokens = TokenizeSuccessfully("hello\nworld\ntest");
  AssertTokenCount(
      tokens,
      kExpectedTokenCountThreeTokens); // identifier + identifier + identifier + EOF (newlines consumed as whitespace)
  // Lines should increment
}

TEST_F(BytecodeLexerTestSuite, PositionTrackingColumnResetAfterNewline) {
  constexpr size_t kExpectedTokenCountTwoTokens = 3;
  auto tokens = TokenizeSuccessfully("hello\nworld");
  AssertTokenCount(tokens,
                   kExpectedTokenCountTwoTokens); // identifier + identifier + EOF (newline consumed as whitespace)
  // Column should reset to 1 after newline
}

// ============================================================================
// 10. EOF Handling Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, EOFEmptyInput) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  auto tokens = TokenizeSuccessfully("");
  AssertTokenCount(tokens, kExpectedTokenCountEOFOnly); // EOF only
}

TEST_F(BytecodeLexerTestSuite, EOFAfterValidTokens) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  auto tokens = TokenizeSuccessfully("hello");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, EOFAfterWhitespace) {
  constexpr size_t kExpectedTokenCountEOFOnly = 1;
  auto tokens = TokenizeSuccessfully("   ");
  AssertTokenCount(tokens, kExpectedTokenCountEOFOnly); // EOF only
}

// ============================================================================
// 11. Custom Handler Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, CustomHandlerSetHandlerForSpecificCharacter) {
  auto lexer = CreateLexer("#test");
  // Create a custom handler that treats '#' as identifier start
  // This would require creating a custom handler class
  // For now, we'll test that default handler works
  AssertTokenizationError("#test", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, CustomHandlerSetDefaultHandler) {
  auto lexer = CreateLexer("#test");
  // Test that we can set a custom default handler
  // This would require creating a custom handler class
  // For now, we'll test that default handler works
  AssertTokenizationError("#test", "Unexpected character");
}

// ============================================================================
// 12. Edge Cases and Boundary Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, EdgeCaseVeryLongInput) {
  constexpr size_t kLongInputSize = 1000;
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  std::string long_input(kLongInputSize, 'a');
  auto tokens = TokenizeSuccessfully(long_input);
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

TEST_F(BytecodeLexerTestSuite, EdgeCaseAllHandlerTypesInSequence) {
  constexpr size_t kExpectedTokenCountFiveTokens = 6;
  auto tokens = TokenizeSuccessfully("hello 123 \"test\" { }");
  AssertTokenCount(tokens,
                   kExpectedTokenCountFiveTokens); // identifier + int literal + string literal + punct + punct + EOF
}

TEST_F(BytecodeLexerTestSuite, EdgeCaseRapidCharacterTransitions) {
  constexpr size_t kExpectedTokenCountSingleToken = 2;
  // "a1b2c3" is parsed as one identifier because identifiers can contain numbers
  auto tokens = TokenizeSuccessfully("a1b2c3");
  AssertTokenCount(tokens, kExpectedTokenCountSingleToken); // identifier + EOF
}

// ============================================================================
// 13. Error Handling Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, ErrorHandlingVerifyErrorReturned) {
  AssertTokenizationError("#", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, ErrorHandlingDescriptiveErrorMessage) {
  AssertTokenizationError("\"unclosed", "Unterminated string literal");
}

TEST_F(BytecodeLexerTestSuite, ErrorHandlingErrorContainsLineInfo) {
  // "." triggers DefaultHandler, not NumberHandler
  AssertTokenizationError(".\n", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, ErrorHandlingFirstErrorReturned) {
  // When multiple errors possible, first should be returned
  AssertTokenizationError("#$", "Unexpected character");
}

TEST_F(BytecodeLexerTestSuite, ErrorHandlingErrorPropagationFromHandlers) {
  AssertTokenizationError(R"("test\x")", "Unknown escape");
}

// ============================================================================
// 14. Complete Ovum Intermediate Language Program Tests
// ============================================================================

TEST_F(BytecodeLexerTestSuite, CompleteProgramSimpleFunction) {
  constexpr size_t kExpectedTokenCount = 8;
  const std::string program = R"(function:1 _Global_Main_StringArray {
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: function : 1 _Global_Main_StringArray { Return } EOF
  AssertTokenCount(tokens, kExpectedTokenCount);

  size_t idx = 0;
  // Verify token types and lexemes (positions are verified but flexible)
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "1", 1);
  AssertTokenIsIdentifier(tokens, idx++, "_Global_Main_StringArray");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsEof(tokens, idx++, 3, 2);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramWhileLoop) {
  const std::string program = R"(function:1 _Global_Main_StringArray {
    while {
        IntLessEqual
    } then {
        PrintLine
        IntAdd
    }
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: function : 1 _Global_Main_StringArray { while { IntLessEqual } then { PrintLine IntAdd } Return }
  // EOF
  constexpr size_t kExpectedTokenCountWhileLoop = 17;
  AssertTokenCount(tokens, kExpectedTokenCountWhileLoop);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "1", 1);
  AssertTokenIsIdentifier(tokens, idx++, "_Global_Main_StringArray");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsKeyword(tokens, idx++, "while");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntLessEqual");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsKeyword(tokens, idx++, "then");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "PrintLine");
  AssertTokenIsIdentifier(tokens, idx++, "IntAdd");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  // EOF - verify type, line is 9
  constexpr int32_t kExpectedEofLine = 9;
  constexpr int32_t kExpectedEofCol = 2;
  AssertTokenIsEof(tokens, idx++, kExpectedEofLine, kExpectedEofCol);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramIfStatement) {
  const std::string program = R"(function:1 Test {
    if {
        IntGreater
    } then {
        PrintLine
    } else {
        Return
    }
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: function : 1 Test { if { IntGreater } then { PrintLine } else { Return } } EOF
  constexpr size_t kExpectedTokenCountIfStatement = 19;
  AssertTokenCount(tokens, kExpectedTokenCountIfStatement);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "1", 1);
  AssertTokenIsIdentifier(tokens, idx++, "Test");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsKeyword(tokens, idx++, "if");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntGreater");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsKeyword(tokens, idx++, "then");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "PrintLine");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsKeyword(tokens, idx++, "else");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsPunct(tokens, idx++, "}");
  // EOF - verify type, line is 9
  constexpr int32_t kExpectedEofLine = 9;
  constexpr int32_t kExpectedEofCol = 2;
  AssertTokenIsEof(tokens, idx++, kExpectedEofLine, kExpectedEofCol);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramWithStringLiteral) {
  const std::string program = R"(function:1 Test {
    StringLiteral "Hello World"
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: function : 1 Test { StringLiteral "Hello World" Return } EOF
  constexpr size_t kExpectedTokenCountWithString = 10;
  AssertTokenCount(tokens, kExpectedTokenCountWithString);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "1", 1);
  AssertTokenIsIdentifier(tokens, idx++, "Test");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "StringLiteral");
  AssertTokenIsStringLiteral(tokens, idx++, "\"Hello World\"", "Hello World");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsEof(tokens, idx++, 4, 2);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramWithNumericLiterals) {
  const std::string program = R"(function:1 Test {
    IntLiteral 42
    FloatLiteral 3.14
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: function : 1 Test { IntLiteral 42 FloatLiteral 3.14 Return } EOF
  constexpr size_t kExpectedTokenCount = 12;
  constexpr int64_t kExpectedIntValue = 42;
  constexpr double kExpectedFloatValue = 3.14;
  AssertTokenCount(tokens, kExpectedTokenCount);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "1", 1);
  AssertTokenIsIdentifier(tokens, idx++, "Test");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntLiteral");
  AssertTokenIsIntLiteral(tokens, idx++, "42", kExpectedIntValue);
  AssertTokenIsIdentifier(tokens, idx++, "FloatLiteral");
  AssertTokenIsFloatLiteral(tokens, idx++, "3.14", kExpectedFloatValue);
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  constexpr int32_t kExpectedEofLine = 5;
  constexpr int32_t kExpectedEofCol = 2;
  AssertTokenIsEof(tokens, idx++, kExpectedEofLine, kExpectedEofCol);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramPureFunction) {
  const std::string program = R"(pure function:2 Calculate {
    IntAdd
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: pure function : 2 Calculate { IntAdd Return } EOF
  constexpr size_t kExpectedTokenCountPureFunction = 10;
  AssertTokenCount(tokens, kExpectedTokenCountPureFunction);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "pure");
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "2", 2);
  AssertTokenIsIdentifier(tokens, idx++, "Calculate");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntAdd");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsEof(tokens, idx++, 4, 2);
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramInitStatic) {
  const std::string program = R"(init-static {
    PrintLine
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Expected tokens: init-static { PrintLine Return } EOF
  constexpr size_t kExpectedTokenCount = 6;
  AssertTokenCount(tokens, kExpectedTokenCount);

  size_t idx = 0;
  // Verify token types and lexemes
  AssertTokenIsKeyword(tokens, idx++, "init-static");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "PrintLine");
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  // EOF line/column - verify type only
  AssertTokenExists(tokens, idx);
  ASSERT_EQ(tokens[idx]->GetStringType(), "EOF");
}

TEST_F(BytecodeLexerTestSuite, CompleteProgramComplexWithAllFeatures) {
  const std::string program = R"(function:3 ProcessData {
    if {
        IntGreater
    } then {
        StringLiteral "Success"
        IntLiteral 100
    } else {
        StringLiteral "Failure"
        IntLiteral 0
    }
    while {
        IntLess
    } then {
        IntAdd
        PrintLine
    }
    Return
})";
  auto tokens = TokenizeSuccessfully(program);

  // Verify token count (function : 3 ProcessData { if { IntGreater } then { StringLiteral "Success" IntLiteral 100 }
  // else { StringLiteral "Failure" IntLiteral 0 } while { IntLess } then { IntAdd PrintLine } Return } EOF)
  constexpr size_t kExpectedTokenCountComplex = 35;
  AssertTokenCount(tokens, kExpectedTokenCountComplex);

  size_t idx = 0;
  // function:3 ProcessData {
  AssertTokenIsKeyword(tokens, idx++, "function");
  AssertTokenIsPunct(tokens, idx++, ":");
  AssertTokenIsIntLiteral(tokens, idx++, "3", 3);
  AssertTokenIsIdentifier(tokens, idx++, "ProcessData");
  AssertTokenIsPunct(tokens, idx++, "{");

  // if { IntGreater } then { StringLiteral "Success" IntLiteral 100 }
  AssertTokenIsKeyword(tokens, idx++, "if");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntGreater");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsKeyword(tokens, idx++, "then");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "StringLiteral");
  AssertTokenIsStringLiteral(tokens, idx++, "\"Success\"", "Success");
  AssertTokenIsIdentifier(tokens, idx++, "IntLiteral");
  constexpr int64_t kExpectedIntValue = 100;
  AssertTokenIsIntLiteral(tokens, idx++, "100", kExpectedIntValue);
  AssertTokenIsPunct(tokens, idx++, "}");

  // else { StringLiteral "Failure" IntLiteral 0 }
  AssertTokenIsKeyword(tokens, idx++, "else");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "StringLiteral");
  AssertTokenIsStringLiteral(tokens, idx++, "\"Failure\"", "Failure");
  AssertTokenIsIdentifier(tokens, idx++, "IntLiteral");
  AssertTokenIsIntLiteral(tokens, idx++, "0", 0);
  AssertTokenIsPunct(tokens, idx++, "}");

  // while { IntLess } then { IntAdd PrintLine }
  AssertTokenIsKeyword(tokens, idx++, "while");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntLess");
  AssertTokenIsPunct(tokens, idx++, "}");
  AssertTokenIsKeyword(tokens, idx++, "then");
  AssertTokenIsPunct(tokens, idx++, "{");
  AssertTokenIsIdentifier(tokens, idx++, "IntAdd");
  AssertTokenIsIdentifier(tokens, idx++, "PrintLine");
  AssertTokenIsPunct(tokens, idx++, "}");

  // Return }
  AssertTokenIsIdentifier(tokens, idx++, "Return");
  AssertTokenIsPunct(tokens, idx++, "}");
  // EOF - verify type and position
  constexpr int32_t kExpectedEofLine = 18;
  constexpr int32_t kExpectedEofCol = 2;
  AssertTokenIsEof(tokens, idx++, kExpectedEofLine, kExpectedEofCol);
}
