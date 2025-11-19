#include "IfParser.hpp"

#include "CommandParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"
#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/ConditionalExecution.hpp"
#include "lib/execution_tree/IfMultibranch.hpp"
#include "lib/execution_tree/command_factory.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> IfParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("if"))
    return std::unexpected(BytecodeParserError("Expected 'if'"));

  ctx.Advance();

  auto if_node = std::make_unique<vm::execution_tree::IfMultibranch>();

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto cond1 = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = cond1.get();

  while (!ctx.IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *cond1);

    if (!res)
      return res;
  }
  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  if (auto e = ctx.ExpectKeyword("then"); !e)
    return std::unexpected(e.error());

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  auto body1 = std::make_unique<vm::execution_tree::Block>();
  ctx.current_block = body1.get();

  while (!ctx.IsPunct('}')) {
    auto res = CommandParser::ParseSingleStatement(ctx, *body1);
    if (!res)
      return res;
  }

  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  if_node->AddBranch(std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(cond1), std::move(body1)));

  while (ctx.IsKeyword("else")) {
    ctx.Advance();

    if (ctx.IsKeyword("if")) {
      ctx.Advance();

      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      auto cond = std::make_unique<vm::execution_tree::Block>();
      ctx.current_block = cond.get();

      while (!ctx.IsPunct('}')) {
        auto res = CommandParser::ParseSingleStatement(ctx, *cond);

        if (!res)
          return res;
      }

      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());

      if (auto e = ctx.ExpectKeyword("then"); !e)
        return std::unexpected(e.error());

      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      auto body = std::make_unique<vm::execution_tree::Block>();
      ctx.current_block = body.get();

      while (!ctx.IsPunct('}')) {
        auto res = CommandParser::ParseSingleStatement(ctx, *body);

        if (!res)
          return res;
      }
      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());

      if_node->AddBranch(std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(cond), std::move(body)));
    } else {
      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      auto else_body = std::make_unique<vm::execution_tree::Block>();
      ctx.current_block = else_body.get();

      while (!ctx.IsPunct('}')) {
        auto res = CommandParser::ParseSingleStatement(ctx, *else_body);

        if (!res)
          return res;
      }

      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());

      auto true_cond = std::make_unique<vm::execution_tree::Block>();
      auto bool_com_res = ovum::vm::execution_tree::CreateBooleanCommandByName("PushBool", true);

      if (!bool_com_res)
        return std::unexpected(BytecodeParserError(bool_com_res.error().what()));

      true_cond->AddStatement(std::move(bool_com_res.value()));
      if_node->AddBranch(
          std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(true_cond), std::move(else_body)));
      break;
    }
  }

  ctx.current_block->AddStatement(std::move(if_node));
  return {};
}

} // namespace ovum::bytecode::parser
