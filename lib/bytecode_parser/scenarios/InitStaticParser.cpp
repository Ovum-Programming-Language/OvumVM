#include "CommandParser.hpp"
#include "InitStaticParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/execution_tree/Block.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> InitStaticParser::Handle(ParsingSessionPtr ctx) {
  if (!ctx->IsKeyword("init-static")) {
    return std::unexpected(BytecodeParserError("Expected 'init-static'", BytecodeParserErrorCode::kNotMatched));
  }

  if (ctx->ReleaseInitStaticBlock() != nullptr) {
    return std::unexpected(BytecodeParserError("Multiple init-static blocks are not allowed"));
  }

  ctx->Advance();

  if (auto e = ctx->ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto block = std::make_unique<vm::execution_tree::Block>();
  ctx->SetCurrentBlock(block.get());

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    auto res = CommandParser::ParseSingleStatement(ctx, *block);
    if (!res)
      return res;
  }

  if (auto e = ctx->ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  ctx->SetInitStaticBlock(std::move(block));
  ctx->SetCurrentBlock(nullptr);

  return {};
}

} // namespace ovum::bytecode::parser
