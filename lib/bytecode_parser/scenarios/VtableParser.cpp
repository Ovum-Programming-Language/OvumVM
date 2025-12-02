#include "VtableParser.hpp"

#include "lib/runtime/VirtualTable.hpp"

#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> VtableParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("vtable")) {
    return std::unexpected(BytecodeParserError("Expected 'vtable'", BytecodeParserErrorCode::kNotMatched));
  }

  ctx->Advance();

  std::expected<std::string, BytecodeParserError> name_res = ctx->ConsumeIdentifier();

  if (!name_res) {
    return std::unexpected(name_res.error());
  }

  std::string class_name = name_res.value();

  std::expected<void, BytecodeParserError> e = ctx->ExpectPunct('{');

  if (!e) {
    return std::unexpected(e.error());
  }

  vm::runtime::VirtualTable vtable(class_name, 0);

  while (!ctx->IsPunct('}') && !ctx->IsEof()) {
    if (ctx->IsKeyword("size")) {
      ctx->Advance();

      e = ctx->ExpectPunct(':');

      if (!e) {
        return std::unexpected(e.error());
      }

      std::expected<int64_t, BytecodeParserError> size_res = ctx->ConsumeIntLiteral();

      if (!size_res) {
        return std::unexpected(size_res.error());
      }

      vtable = vm::runtime::VirtualTable(class_name, static_cast<size_t>(size_res.value()));
    } else if (ctx->IsKeyword("interfaces")) {
      ctx->Advance();

      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      while (!ctx->IsPunct('}')) {
        std::expected<std::string, BytecodeParserError> iface = ctx->ConsumeIdentifier();

        if (!iface) {
          return std::unexpected(iface.error());
        }

        vtable.AddInterface(iface.value());

        if (ctx->IsPunct(',')) {
          ctx->Advance();
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }
    } else if (ctx->IsKeyword("methods")) {
      ctx->Advance();

      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      while (!ctx->IsPunct('}')) {
        std::expected<std::string, BytecodeParserError> virt = ctx->ConsumeIdentifier();

        if (!virt) {
          return std::unexpected(virt.error());
        }

        e = ctx->ExpectPunct(':');

        if (!e) {
          return std::unexpected(e.error());
        }

        std::expected<std::string, BytecodeParserError> real = ctx->ConsumeIdentifier();

        if (!real) {
          return std::unexpected(real.error());
        }

        vtable.AddFunction(virt.value(), real.value());

        if (ctx->IsPunct(',')) {
          ctx->Advance();
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }
    } else if (ctx->IsKeyword("vartable")) {
      ctx->Advance();

      e = ctx->ExpectPunct('{');

      if (!e) {
        return std::unexpected(e.error());
      }

      while (!ctx->IsPunct('}')) {
        std::expected<std::string, BytecodeParserError> field = ctx->ConsumeIdentifier();

        if (!field) {
          return std::unexpected(field.error());
        }

        e = ctx->ExpectPunct(':');

        if (!e) {
          return std::unexpected(e.error());
        }

        std::expected<std::string, BytecodeParserError> type = ctx->ConsumeIdentifier();

        if (!type) {
          return std::unexpected(type.error());
        }

        e = ctx->ExpectPunct('@');

        if (!e) {
          return std::unexpected(e.error());
        }

        std::expected<int64_t, BytecodeParserError> offset = ctx->ConsumeIntLiteral();

        if (!offset) {
          return std::unexpected(offset.error());
        }

        vtable.AddField(type.value(), offset.value());

        if (ctx->IsPunct(',')) {
          ctx->Advance();
        }
      }

      e = ctx->ExpectPunct('}');

      if (!e) {
        return std::unexpected(e.error());
      }
    } else {
      return std::unexpected(BytecodeParserError("Unknown vtable directive: " + ctx->Current()->GetLexeme()));
    }
  }

  e = ctx->ExpectPunct('}');

  if (!e) {
    return std::unexpected(e.error());
  }

  if (!ctx->VTableRepo().Add(std::move(vtable))) {
    return std::unexpected(BytecodeParserError("Failed to add vtable"));
  }

  return {};
}

} // namespace ovum::bytecode::parser
