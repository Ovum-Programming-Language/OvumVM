#include "InitStaticParser.hpp"

#include "lib/execution_tree/Block.hpp"

#include "CommandFactory.hpp"
#include "CommandParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

InitStaticParser::InitStaticParser(const ICommandFactory& factory) : factory_(std::move(factory)) {
}

std::expected<bool, BytecodeParserError> InitStaticParser::Handle(ParsingSessionPtr ctx) {
  if (!ctx->IsKeyword("init-static")) {
    return false;
  }

  if (ctx->ReleaseInitStaticBlock() != nullptr) {
    return std::unexpected(BytecodeParserError("Multiple init-static blocks are not allowed"));
  }

  ctx->Advance();

  std::expected<void, BytecodeParserError> e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> block = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(block.get());

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *block, factory_);

    if (!res) {
      return res;
    } else if (!res.value()) {
      return std::unexpected(
          BytecodeParserError("Command expected at line" + std::to_string(ctx->Current()->GetPosition().GetLine())));
    }
  }

  e = ctx->ExpectPunct('}');

  if (!e) {
    return std::unexpected(e.error());
  }

  ctx->SetInitStaticBlock(std::move(block));
  ctx->SetCurrentBlock(nullptr);

  return true;
}

} // namespace ovum::bytecode::parser
