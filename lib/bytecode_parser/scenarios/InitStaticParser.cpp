#include "InitStaticParser.hpp"

#include "CommandParser.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/PassedExecutionData.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> InitStaticParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("init-static"))
    return std::unexpected(BytecodeParserError("Expected 'init-static'"));

  if (ctx.init_static_parsed) {
    return std::unexpected(BytecodeParserError("Multiple init-static blocks are not allowed"));
  }

  ctx.Advance();

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto block = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = block.get();

  while (!ctx.IsPunct('}') && !ctx.IsEof()) {
    auto res = CommandParser::ParseSingleStatement(ctx, *block);
    if (!res)
      return res;
  }

  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  vm::execution_tree::PassedExecutionData exec_data{
      .memory = ctx.memory, .virtual_table_repository = ctx.vtable_repo, .function_repository = ctx.func_repo};

  auto result = block->Execute(exec_data);
  if (!result) {
    return std::unexpected(BytecodeParserError("Runtime error in init-static: " + std::string(result.error().what())));
  }

  ctx.init_static_parsed = true;
  ctx.current_block = nullptr;
  return {};
}

} // namespace ovum::bytecode::parser
