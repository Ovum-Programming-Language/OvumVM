#include "VtableParser.hpp"

#include "lib/bytecode_parser/ParserContext.hpp"
#include "lib/runtime/VirtualTable.hpp"

namespace ovum::bytecode::parser {

bool VtableParser::Handle(ParserContext& ctx) {
  if (!ctx.IsKeyword("vtable"))
    return false;

  ctx.Advance();
  auto name_res = ctx.ConsumeIdentifier();
  if (!name_res)
    throw name_res.error();
  std::string class_name = name_res.value();

  if (auto e = ctx.ExpectPunct('{'); !e)
    throw e.error();

  vm::runtime::VirtualTable vtable(class_name, 0);

  while (!ctx.IsPunct('}') && !ctx.IsEof()) {
    if (ctx.IsKeyword("size:")) {
      ctx.Advance();
      auto size_res = ctx.ConsumeIntLiteral();
      if (!size_res)
        throw size_res.error();
      vtable = vm::runtime::VirtualTable(class_name, static_cast<size_t>(size_res.value()));
    } else if (ctx.IsKeyword("interfaces")) {
      ctx.Advance();
      if (auto e = ctx.ExpectPunct('{'); !e)
        throw e.error();
      while (!ctx.IsPunct('}')) {
        auto iface = ctx.ConsumeIdentifier();
        if (!iface)
          break;
        vtable.AddInterface(iface.value());
        if (ctx.IsPunct(','))
          ctx.Advance();
      }
      if (auto e = ctx.ExpectPunct('}'); !e)
        throw e.error();
    } else if (ctx.IsKeyword("methods")) {
      ctx.Advance();
      if (auto e = ctx.ExpectPunct('{'); !e)
        throw e.error();
      while (!ctx.IsPunct('}')) {
        auto virt = ctx.ConsumeIdentifier();
        if (!virt)
          break;
        if (auto e = ctx.ExpectPunct(':'); !e)
          throw e.error();
        auto real = ctx.ConsumeIdentifier();
        if (!real)
          throw real.error();
        vtable.AddFunction(virt.value(), real.value());
        if (ctx.IsPunct(','))
          ctx.Advance();
      }
      if (auto e = ctx.ExpectPunct('}'); !e)
        throw e.error();
    } else if (ctx.IsKeyword("vartable")) {
      ctx.Advance();
      if (auto e = ctx.ExpectPunct('{'); !e)
        throw e.error();
      while (!ctx.IsPunct('}')) {
        auto field = ctx.ConsumeIdentifier();
        if (!field)
          break;
        if (auto e = ctx.ExpectPunct(':'); !e)
          throw e.error();
        auto type = ctx.ConsumeIdentifier();
        if (!type)
          throw type.error();
        if (auto e = ctx.ExpectPunct('@'); !e)
          throw e.error();
        auto offset = ctx.ConsumeIntLiteral();
        if (!offset)
          throw offset.error();
        vtable.AddField(type.value(), static_cast<size_t>(offset.value()));
        if (ctx.IsPunct(','))
          ctx.Advance();
      }
      if (auto e = ctx.ExpectPunct('}'); !e)
        throw e.error();
    } else {
      throw BytecodeParserError("Unknown vtable directive: " + ctx.Current()->GetLexeme());
    }
  }

  if (auto e = ctx.ExpectPunct('}'); !e)
    throw e.error();

  if (!ctx.vtable_repo.Add(std::move(vtable))) {
    throw BytecodeParserError("Failed to add vtable");
  }

  return true;
}

} // namespace ovum::bytecode::parser
