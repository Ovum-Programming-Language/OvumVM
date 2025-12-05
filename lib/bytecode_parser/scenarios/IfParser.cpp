#include "IfParser.hpp"

#include "lib/execution_tree/Block.hpp"
#include "lib/execution_tree/ConditionalExecution.hpp"
#include "lib/execution_tree/IfMultibranch.hpp"
#include "lib/execution_tree/command_factory.hpp"

#include "CommandParser.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

IfParser::IfParser(const ICommandFactory& factory) : factory_(factory) {
}

std::expected<bool, BytecodeParserError> IfParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("if")) {
    return false;
  }

  ctx->Advance();

  vm::execution_tree::Block* parent_block = ctx->GetCurrentBlock();

  std::unique_ptr<vm::execution_tree::IfMultibranch> if_node = std::make_unique<vm::execution_tree::IfMultibranch>();

  std::expected<void, BytecodeParserError> e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> cond1 = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(cond1.get());

  while (!ctx->IsPunct('}')) {
    std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *cond1, factory_);

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

  e = ctx->ExpectKeyword("then");

  if (!e) {
    return std::unexpected(e.error());
  }

  e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  std::unique_ptr<vm::execution_tree::Block> body1 = std::make_unique<vm::execution_tree::Block>();

  ctx->SetCurrentBlock(body1.get());

  while (!ctx->IsPunct('}')) {
    std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *body1, factory_);

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

  if_node->AddBranch(std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(cond1), std::move(body1)));

  while (ctx->IsKeyword("else")) {
    ctx->Advance();

    if (ctx->IsKeyword("if")) {
      ctx->Advance();

      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      std::unique_ptr<vm::execution_tree::Block> cond = std::make_unique<vm::execution_tree::Block>();

      ctx->SetCurrentBlock(cond.get());

      while (!ctx->IsPunct('}')) {
        std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *cond, factory_);

        if (!res) {
          return res;
        } else if (!res.value()) {
          return std::unexpected(BytecodeParserError("Command expected at line" +
                                                     std::to_string(ctx->Current()->GetPosition().GetLine())));
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }

      e = ctx->ExpectKeyword("then");

      if (!e) {
        return std::unexpected(e.error());
      }

      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      std::unique_ptr<vm::execution_tree::Block> body = std::make_unique<vm::execution_tree::Block>();

      ctx->SetCurrentBlock(body.get());

      while (!ctx->IsPunct('}')) {
        std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *body, factory_);

        if (!res) {
          return res;
        } else if (!res.value()) {
          return std::unexpected(BytecodeParserError("Command expected at line" +
                                                     std::to_string(ctx->Current()->GetPosition().GetLine())));
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }

      if_node->AddBranch(std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(cond), std::move(body)));
    } else {
      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      std::unique_ptr<vm::execution_tree::Block> else_body = std::make_unique<vm::execution_tree::Block>();

      ctx->SetCurrentBlock(else_body.get());

      while (!ctx->IsPunct('}')) {
        std::expected<bool, BytecodeParserError> res = CommandParser::ParseSingleStatement(ctx, *else_body, factory_);

        if (!res) {
          return res;
        } else if (!res.value()) {
          return std::unexpected(BytecodeParserError("Command expected at line" +
                                                     std::to_string(ctx->Current()->GetPosition().GetLine())));
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }

      std::unique_ptr<vm::execution_tree::Block> true_cond = std::make_unique<vm::execution_tree::Block>();

      std::expected<std::unique_ptr<vm::execution_tree::IExecutable>, std::out_of_range> bool_com_res =
          ovum::vm::execution_tree::CreateBooleanCommandByName("PushBool", true);

      if (!bool_com_res) {
        return std::unexpected(BytecodeParserError(bool_com_res.error().what()));
      }

      true_cond->AddStatement(std::move(bool_com_res.value()));

      if_node->AddBranch(
          std::make_unique<vm::execution_tree::ConditionalExecution>(std::move(true_cond), std::move(else_body)));

      break;
    }
  }

  parent_block->AddStatement(std::move(if_node));
  ctx->SetCurrentBlock(parent_block);

  return {};
}

} // namespace ovum::bytecode::parser
