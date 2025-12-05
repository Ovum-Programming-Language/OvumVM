#include "BytecodeLexerTestSuite.hpp"

#include <cassert>

#include "lib/bytecode_lexer/BytecodeLexerError.hpp"

void BytecodeLexerTestSuite::SetUp() {
  // No special setup needed
}

void BytecodeLexerTestSuite::TearDown() {
  // No special teardown needed
}

ovum::bytecode::lexer::BytecodeLexer BytecodeLexerTestSuite::CreateLexer(std::string_view src) {
  return ovum::bytecode::lexer::BytecodeLexer(src);
}

void BytecodeLexerTestSuite::AssertTokenizationSuccess(const std::string& input, size_t expected_token_count) {
  auto lexer = CreateLexer(input);
  auto result = lexer.Tokenize();
  ASSERT_TRUE(result.has_value()) << "Tokenization should succeed for: " << input;
  AssertTokenCount(result.value(), expected_token_count);
}

void BytecodeLexerTestSuite::AssertTokenizationError(const std::string& input,
                                                     const std::string& expected_error_substring) {
  auto lexer = CreateLexer(input);
  auto result = lexer.Tokenize();
  ASSERT_FALSE(result.has_value()) << "Tokenization should fail for: " << input;
  std::string error_msg = result.error().what();
  ASSERT_NE(error_msg.find(expected_error_substring), std::string::npos)
      << "Error message should contain '" << expected_error_substring << "', but got: " << error_msg;
}

void BytecodeLexerTestSuite::AssertTokenCount(const std::vector<ovum::TokenPtr>& tokens, size_t expected_count) {
  ASSERT_EQ(tokens.size(), expected_count) << "Expected " << expected_count << " tokens, got " << tokens.size();
}

void BytecodeLexerTestSuite::AssertTokenExists(const std::vector<ovum::TokenPtr>& tokens, size_t index) {
  ASSERT_LT(index, tokens.size()) << "Token index " << index << " out of range (size: " << tokens.size() << ")";
  ASSERT_NE(tokens[index], nullptr) << "Token at index " << index << " is null";
}

std::vector<ovum::TokenPtr> BytecodeLexerTestSuite::TokenizeSuccessfully(const std::string& input) {
  auto lexer = CreateLexer(input);
  auto result = lexer.Tokenize();
  EXPECT_TRUE(result.has_value()) << "Tokenization should succeed for: " << input;
  if (!result.has_value()) {
    return {};
  }
  return result.value();
}

// Token assertion helpers - implemented using Token API

void BytecodeLexerTestSuite::AssertTokenIsEof(const std::vector<ovum::TokenPtr>& tokens,
                                              size_t index,
                                              int32_t expected_line,
                                              int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "EOF") << "Token at index " << index << " should be EOF";
  ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "EOF token should be at line " << expected_line;
  ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "EOF token should be at column " << expected_col;
}

void BytecodeLexerTestSuite::AssertTokenIsNewline(const std::vector<ovum::TokenPtr>& tokens,
                                                  size_t index,
                                                  int32_t expected_line,
                                                  int32_t expected_col) {
  AssertTokenExists(tokens, index);
  // Note: NewlineHandler was removed, so this is not used, but kept for API compatibility
  ASSERT_EQ(tokens[index]->GetStringType(), "NEWLINE") << "Token at index " << index << " should be NEWLINE";
}

void BytecodeLexerTestSuite::AssertTokenIsIdentifier(const std::vector<ovum::TokenPtr>& tokens,
                                                     size_t index,
                                                     const std::string& expected_value,
                                                     int32_t expected_line,
                                                     int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "IDENT") << "Token at index " << index << " should be IDENT";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_value)
      << "Token at index " << index << " should have lexeme '" << expected_value << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}

void BytecodeLexerTestSuite::AssertTokenIsKeyword(const std::vector<ovum::TokenPtr>& tokens,
                                                  size_t index,
                                                  const std::string& expected_value,
                                                  int32_t expected_line,
                                                  int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "KEYWORD") << "Token at index " << index << " should be KEYWORD";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_value)
      << "Token at index " << index << " should have lexeme '" << expected_value << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}

void BytecodeLexerTestSuite::AssertTokenIsIntLiteral(const std::vector<ovum::TokenPtr>& tokens,
                                                     size_t index,
                                                     const std::string& expected_lexeme,
                                                     int64_t expected_value,
                                                     int32_t expected_line,
                                                     int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "LITERAL:Int") << "Token at index " << index << " should be LITERAL:Int";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_lexeme)
      << "Token at index " << index << " should have lexeme '" << expected_lexeme << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}

void BytecodeLexerTestSuite::AssertTokenIsFloatLiteral(const std::vector<ovum::TokenPtr>& tokens,
                                                       size_t index,
                                                       const std::string& expected_lexeme,
                                                       double expected_value,
                                                       int32_t expected_line,
                                                       int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "LITERAL:Float")
      << "Token at index " << index << " should be LITERAL:Float";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_lexeme)
      << "Token at index " << index << " should have lexeme '" << expected_lexeme << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}

void BytecodeLexerTestSuite::AssertTokenIsStringLiteral(const std::vector<ovum::TokenPtr>& tokens,
                                                        size_t index,
                                                        const std::string& expected_raw,
                                                        const std::string& expected_value,
                                                        int32_t expected_line,
                                                        int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "LITERAL:String")
      << "Token at index " << index << " should be LITERAL:String";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_raw)
      << "Token at index " << index << " should have raw lexeme '" << expected_raw << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}

void BytecodeLexerTestSuite::AssertTokenIsPunct(const std::vector<ovum::TokenPtr>& tokens,
                                                size_t index,
                                                const std::string& expected_value,
                                                int32_t expected_line,
                                                int32_t expected_col) {
  AssertTokenExists(tokens, index);
  ASSERT_EQ(tokens[index]->GetStringType(), "PUNCT") << "Token at index " << index << " should be PUNCT";
  ASSERT_EQ(tokens[index]->GetLexeme(), expected_value)
      << "Token at index " << index << " should have lexeme '" << expected_value << "'";
  if (expected_line >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetLine(), expected_line) << "Token should be at line " << expected_line;
  }
  if (expected_col >= 0) {
    ASSERT_EQ(tokens[index]->GetPosition().GetColumn(), expected_col) << "Token should be at column " << expected_col;
  }
}
