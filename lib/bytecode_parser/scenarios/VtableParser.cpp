#include "VtableParser.hpp"

#include "lib/runtime/VirtualTable.hpp"

#include "FunctionFactory.hpp"
#include "lib/bytecode_parser/BytecodeParserError.hpp"

namespace ovum::bytecode::parser {

std::expected<bool, BytecodeParserError> VtableParser::Handle(std::shared_ptr<ParsingSession> ctx) {
  if (!ctx->IsKeyword("vtable")) {
    return false;
  }

  ctx->Advance();

  std::expected<std::string, BytecodeParserError> name_res = ctx->ConsumeIdentifier();

  if (!name_res) {
    return std::unexpected(name_res.error());
  }

  std::string class_name = name_res.value();

  const std::string dtor_virtual_name = class_name + "_destructor_<M>";
  const std::string dtor_real_name = class_name + "_destructor_<M>";
  bool has_destructor = false;

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

        std::string virt_name = virt.value();
        std::string real_name = real.value();

        if (virt_name == dtor_virtual_name || real_name == dtor_real_name) {
          has_destructor = true;
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

  // Check if vtable with this name already exists before adding
  auto existing_vtable = ctx->GetVTableRepo().GetByName(class_name);
  if (existing_vtable.has_value()) {
    return std::unexpected(BytecodeParserError("Failed to add vtable"));
  }

  if (!has_destructor) {
    auto body = std::make_unique<vm::execution_tree::Block>();

    constexpr size_t kDestructorArity = 1;

    FunctionFactory factory(ctx->GetJitFactory(), ctx->GetJitBoundary());

    std::unique_ptr<vm::execution_tree::IFunctionExecutable> dtor_func =
        factory.Create(dtor_real_name, kDestructorArity, std::move(body), false, {}, false);

    if (dtor_func == nullptr) {
      return std::unexpected(BytecodeParserError("Failed to create autogenerated destructor: JIT compilation failed"));
    }

    auto add_res = ctx->GetFuncRepo().Add(std::move(dtor_func));

    if (!add_res) {
      return std::unexpected(
          BytecodeParserError(std::string("Failed to add autogenerated destructor: ") + add_res.error().what()));
    }

    vtable.AddFunction(dtor_virtual_name, dtor_real_name);
  }

  if (!ctx->GetVTableRepo().Add(std::move(vtable))) {
    return std::unexpected(BytecodeParserError("Failed to add vtable"));
  }

  return true;
}

} // namespace ovum::bytecode::parser
