#include "VtableParser.hpp"

#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::bytecode::parser {

std::expected<void, BytecodeParserError> VtableParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("vtable"))
    return std::unexpected(BytecodeParserError("Expected 'vtable'"));

  ctx.Advance();

  auto name_res = ctx.ConsumeIdentifier();

  if (!name_res)
    return std::unexpected(name_res.error());

  std::string class_name = name_res.value();

  if (auto e = ctx.ExpectPunct('{'); !e)
    return std::unexpected(e.error());

  vm::runtime::VirtualTable vtable(class_name, 0);

  while (!ctx.IsPunct('}') && !ctx.IsEof()) {
    if (ctx.IsKeyword("size:")) {
      ctx.Advance();

      auto size_res = ctx.ConsumeIntLiteral();

      if (!size_res)
        return std::unexpected(size_res.error());

      vtable = vm::runtime::VirtualTable(class_name, static_cast<size_t>(size_res.value()));
    } else if (ctx.IsKeyword("interfaces")) {
      ctx.Advance();

      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      while (!ctx.IsPunct('}')) {
        auto iface = ctx.ConsumeIdentifier();

        if (!iface)
          return std::unexpected(iface.error());

        vtable.AddInterface(iface.value());

        if (ctx.IsPunct(','))
          ctx.Advance();
      }

      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());
    } else if (ctx.IsKeyword("methods")) {
      ctx.Advance();

      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      while (!ctx.IsPunct('}')) {
        auto virt = ctx.ConsumeIdentifier();

        if (!virt)
          return std::unexpected(virt.error());

        if (auto e = ctx.ExpectPunct(':'); !e)
          return std::unexpected(e.error());

        auto real = ctx.ConsumeIdentifier();

        if (!real)
          return std::unexpected(real.error());

        vtable.AddFunction(virt.value(), real.value());

        if (ctx.IsPunct(','))
          ctx.Advance();
      }

      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());

    } else if (ctx.IsKeyword("vartable")) {
      ctx.Advance();

      if (auto e = ctx.ExpectPunct('{'); !e)
        return std::unexpected(e.error());

      while (!ctx.IsPunct('}')) {
        auto field = ctx.ConsumeIdentifier();

        if (!field)
          return std::unexpected(field.error());

        if (auto e = ctx.ExpectPunct(':'); !e)
          return std::unexpected(e.error());

        auto type = ctx.ConsumeIdentifier();

        if (!type)
          return std::unexpected(type.error());

        if (auto e = ctx.ExpectPunct('@'); !e)
          return std::unexpected(e.error());

        auto offset = ctx.ConsumeIntLiteral();

        if (!offset)
          return std::unexpected(offset.error());

        vtable.AddField(type.value(), static_cast<size_t>(offset.value()));

        if (ctx.IsPunct(','))
          ctx.Advance();
      }
      if (auto e = ctx.ExpectPunct('}'); !e)
        return std::unexpected(e.error());
    } else {
      return std::unexpected(BytecodeParserError("Unknown vtable directive: " + ctx.Current()->GetLexeme()));
    }
  }

  if (auto e = ctx.ExpectPunct('}'); !e)
    return std::unexpected(e.error());

  if (!ctx.vtable_repo.Add(std::move(vtable))) {
    return std::unexpected(BytecodeParserError("Failed to add vtable"));
  }

  return {};
}

} // namespace ovum::bytecode::parser
