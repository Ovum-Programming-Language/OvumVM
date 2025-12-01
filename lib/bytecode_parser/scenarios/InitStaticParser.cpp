#include "InitStaticParser.hpp"

#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"

#include "CommandParser.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> InitStaticParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("init-static")) {
    return std::unexpected(BytecodeParserError("Expected 'init-static'", BytecodeParserErrorCode::kNotMatched));
  }

  if (ctx->init_static_parsed) {
    return std::unexpected(BytecodeParserError("Multiple init-static blocks are not allowed"));
  }

  ctx->Advance();

  if (auto e = ctx->ExpectPunct('{'); !e) {
    return std::unexpected(e.error());
  }

  auto block = std::make_unique<vm::execution_tree::Block>();
  ctx->current_block = block.get();

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    auto res = CommandParser::ParseSingleStatement(ctx, *block);
    if (!res) return res;
  }

  if (auto e = ctx->ExpectPunct('}'); !e) {
    return std::unexpected(e.error());
  }

  ctx->init_static_block = std::move(block);
  ctx->init_static_parsed = true;
  ctx->current_block = nullptr;

  return {};
}

} // namespace ovum::bytecode::parser
