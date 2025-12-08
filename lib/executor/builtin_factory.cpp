#include "builtin_factory.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "lib/execution_tree/Command.hpp"
#include "lib/execution_tree/Function.hpp"
#include "lib/execution_tree/FunctionRepository.hpp"
#include "lib/execution_tree/IFunctionExecutable.hpp"
#include "lib/executor/BuiltinFunctions.hpp"
#include "lib/runtime/FunctionId.hpp"
#include "lib/runtime/ObjectDescriptor.hpp"
#include "lib/runtime/VirtualTable.hpp"
#include "lib/runtime/VirtualTableRepository.hpp"

namespace ovum::vm::runtime {

// Helper to create a Command function for a method
template<typename Func>
std::unique_ptr<execution_tree::IFunctionExecutable> CreateMethodFunction(const std::string& function_id,
                                                                          size_t arity,
                                                                          Func func) {
  auto command = std::make_unique<execution_tree::Command<Func>>(std::move(func));
  return std::make_unique<execution_tree::Function>(function_id, arity, std::move(command));
}

std::expected<void, std::runtime_error> RegisterBuiltinVirtualTables(VirtualTableRepository& repository) {
  // Int: wrapper around int64_t
  {
    VirtualTable int_vtable("Int", sizeof(ObjectDescriptor) + sizeof(int64_t));
    int_vtable.AddField("int", sizeof(ObjectDescriptor));
    int_vtable.AddFunction("_destructor_<M>", "_Int_destructor_<M>");
    int_vtable.AddFunction("_Equals_<C>_IComparable", "_Int_Equals_<C>_IComparable");
    int_vtable.AddFunction("_IsLess_<C>_IComparable", "_Int_IsLess_<C>_IComparable");
    int_vtable.AddFunction("_ToString_<C>", "_Int_ToString_<C>");
    int_vtable.AddFunction("_GetHash_<C>", "_Int_GetHash_<C>");
    int_vtable.AddInterface("IComparable");
    int_vtable.AddInterface("IHashable");
    int_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(int_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Float: wrapper around double
  {
    VirtualTable float_vtable("Float", sizeof(ObjectDescriptor) + sizeof(double));
    float_vtable.AddField("float", sizeof(ObjectDescriptor));
    float_vtable.AddFunction("_destructor_<M>", "_Float_destructor_<M>");
    float_vtable.AddFunction("_Equals_<C>_IComparable", "_Float_Equals_<C>_IComparable");
    float_vtable.AddFunction("_IsLess_<C>_IComparable", "_Float_IsLess_<C>_IComparable");
    float_vtable.AddFunction("_ToString_<C>", "_Float_ToString_<C>");
    float_vtable.AddFunction("_GetHash_<C>", "_Float_GetHash_<C>");
    float_vtable.AddInterface("IComparable");
    float_vtable.AddInterface("IHashable");
    float_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(float_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Char: wrapper around char
  {
    VirtualTable char_vtable("Char", sizeof(ObjectDescriptor) + sizeof(char));
    char_vtable.AddField("char", sizeof(ObjectDescriptor));
    char_vtable.AddFunction("_destructor_<M>", "_Char_destructor_<M>");
    char_vtable.AddFunction("_Equals_<C>_IComparable", "_Char_Equals_<C>_IComparable");
    char_vtable.AddFunction("_IsLess_<C>_IComparable", "_Char_IsLess_<C>_IComparable");
    char_vtable.AddFunction("_ToString_<C>", "_Char_ToString_<C>");
    char_vtable.AddFunction("_GetHash_<C>", "_Char_GetHash_<C>");
    char_vtable.AddInterface("IComparable");
    char_vtable.AddInterface("IHashable");
    char_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(char_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Byte: wrapper around uint8_t
  {
    VirtualTable byte_vtable("Byte", sizeof(ObjectDescriptor) + sizeof(uint8_t));
    byte_vtable.AddField("byte", sizeof(ObjectDescriptor));
    byte_vtable.AddFunction("_destructor_<M>", "_Byte_destructor_<M>");
    byte_vtable.AddFunction("_Equals_<C>_IComparable", "_Byte_Equals_<C>_IComparable");
    byte_vtable.AddFunction("_IsLess_<C>_IComparable", "_Byte_IsLess_<C>_IComparable");
    byte_vtable.AddFunction("_ToString_<C>", "_Byte_ToString_<C>");
    byte_vtable.AddFunction("_GetHash_<C>", "_Byte_GetHash_<C>");
    byte_vtable.AddInterface("IComparable");
    byte_vtable.AddInterface("IHashable");
    byte_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(byte_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Bool: wrapper around bool
  {
    VirtualTable bool_vtable("Bool", sizeof(ObjectDescriptor) + sizeof(bool));
    bool_vtable.AddField("bool", sizeof(ObjectDescriptor));
    bool_vtable.AddFunction("_destructor_<M>", "_Bool_destructor_<M>");
    bool_vtable.AddFunction("_Equals_<C>_IComparable", "_Bool_Equals_<C>_IComparable");
    bool_vtable.AddFunction("_IsLess_<C>_IComparable", "_Bool_IsLess_<C>_IComparable");
    bool_vtable.AddFunction("_ToString_<C>", "_Bool_ToString_<C>");
    bool_vtable.AddFunction("_GetHash_<C>", "_Bool_GetHash_<C>");
    bool_vtable.AddInterface("IComparable");
    bool_vtable.AddInterface("IHashable");
    bool_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(bool_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Nullable: wrapper around Object (void*)
  {
    VirtualTable nullable_vtable("Nullable", sizeof(ObjectDescriptor) + sizeof(void*));
    nullable_vtable.AddField("Object", sizeof(ObjectDescriptor));
    nullable_vtable.AddFunction("_destructor_<M>", "_Nullable_destructor_<M>");
    auto result = repository.Add(std::move(nullable_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // String: wrapper around std::string
  {
    VirtualTable string_vtable("String", sizeof(ObjectDescriptor) + sizeof(std::string));
    string_vtable.AddField("Object", sizeof(ObjectDescriptor));
    string_vtable.AddFunction("_destructor_<M>", "_String_destructor_<M>");
    string_vtable.AddFunction("_Equals_<C>_IComparable", "_String_Equals_<C>_IComparable");
    string_vtable.AddFunction("_IsLess_<C>_IComparable", "_String_IsLess_<C>_IComparable");
    string_vtable.AddFunction("_ToString_<C>", "_String_ToString_<C>");
    string_vtable.AddFunction("_GetHash_<C>", "_String_GetHash_<C>");
    string_vtable.AddFunction("_Length_<C>", "_String_Length_<C>");
    string_vtable.AddFunction("_ToUtf8Bytes_<C>", "_String_ToUtf8Bytes_<C>");
    string_vtable.AddInterface("IComparable");
    string_vtable.AddInterface("IHashable");
    string_vtable.AddInterface("IStringConvertible");
    auto result = repository.Add(std::move(string_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // File: wrapper around std::fstream
  {
    VirtualTable file_vtable("File", sizeof(ObjectDescriptor) + sizeof(std::fstream));
    file_vtable.AddField("Object", sizeof(ObjectDescriptor));
    file_vtable.AddFunction("_destructor_<M>", "_File_destructor_<M>");
    file_vtable.AddFunction("_Open_<M>_String_String", "_File_Open_<M>_String_String");
    file_vtable.AddFunction("_Close_<M>", "_File_Close_<M>");
    file_vtable.AddFunction("_IsOpen_<C>", "_File_IsOpen_<C>");
    file_vtable.AddFunction("_Read_<M>_Int", "_File_Read_<M>_Int");
    file_vtable.AddFunction("_Write_<M>_ByteArray", "_File_Write_<M>_ByteArray");
    file_vtable.AddFunction("_ReadLine_<M>", "_File_ReadLine_<M>");
    file_vtable.AddFunction("_WriteLine_<M>_String", "_File_WriteLine_<M>_String");
    file_vtable.AddFunction("_Seek_<M>_Int", "_File_Seek_<M>_Int");
    file_vtable.AddFunction("_Tell_<C>", "_File_Tell_<C>");
    file_vtable.AddFunction("_Eof_<C>", "_File_Eof_<C>");
    // File does not implement IComparable or IHashable (files cannot be meaningfully compared or hashed)
    auto result = repository.Add(std::move(file_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // IntArray: wrapper around std::vector<int64_t>
  {
    VirtualTable int_array_vtable("IntArray", sizeof(ObjectDescriptor) + sizeof(std::vector<int64_t>));
    int_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    int_array_vtable.AddFunction("_destructor_<M>", "_IntArray_destructor_<M>");
    int_array_vtable.AddFunction("_Equals_<C>_IComparable", "_IntArray_Equals_<C>_IComparable");
    int_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_IntArray_IsLess_<C>_IComparable");
    int_array_vtable.AddFunction("_GetHash_<C>", "_IntArray_GetHash_<C>");
    int_array_vtable.AddInterface("IComparable");
    int_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(int_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // FloatArray: wrapper around std::vector<double>
  {
    VirtualTable float_array_vtable("FloatArray", sizeof(ObjectDescriptor) + sizeof(std::vector<double>));
    float_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    float_array_vtable.AddFunction("_destructor_<M>", "_FloatArray_destructor_<M>");
    float_array_vtable.AddFunction("_Equals_<C>_IComparable", "_FloatArray_Equals_<C>_IComparable");
    float_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_FloatArray_IsLess_<C>_IComparable");
    float_array_vtable.AddFunction("_GetHash_<C>", "_FloatArray_GetHash_<C>");
    float_array_vtable.AddInterface("IComparable");
    float_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(float_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // CharArray: wrapper around std::vector<char>
  {
    VirtualTable char_array_vtable("CharArray", sizeof(ObjectDescriptor) + sizeof(std::vector<char>));
    char_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    char_array_vtable.AddFunction("_destructor_<M>", "_CharArray_destructor_<M>");
    char_array_vtable.AddFunction("_Equals_<C>_IComparable", "_CharArray_Equals_<C>_IComparable");
    char_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_CharArray_IsLess_<C>_IComparable");
    char_array_vtable.AddFunction("_GetHash_<C>", "_CharArray_GetHash_<C>");
    char_array_vtable.AddInterface("IComparable");
    char_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(char_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // ByteArray: wrapper around std::vector<uint8_t>
  // Special handling for byte view casting
  {
    VirtualTable byte_array_vtable("ByteArray", sizeof(ObjectDescriptor) + sizeof(std::vector<uint8_t>));
    byte_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    byte_array_vtable.AddFunction("_destructor_<M>", "_ByteArray_destructor_<M>");
    byte_array_vtable.AddFunction("_Equals_<C>_IComparable", "_ByteArray_Equals_<C>_IComparable");
    byte_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_ByteArray_IsLess_<C>_IComparable");
    byte_array_vtable.AddFunction("_GetHash_<C>", "_ByteArray_GetHash_<C>");
    byte_array_vtable.AddInterface("IComparable");
    byte_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(byte_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // BoolArray: wrapper around std::vector<bool>
  {
    VirtualTable bool_array_vtable("BoolArray", sizeof(ObjectDescriptor) + sizeof(std::vector<bool>));
    bool_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    bool_array_vtable.AddFunction("_destructor_<M>", "_BoolArray_destructor_<M>");
    bool_array_vtable.AddFunction("_Equals_<C>_IComparable", "_BoolArray_Equals_<C>_IComparable");
    bool_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_BoolArray_IsLess_<C>_IComparable");
    bool_array_vtable.AddFunction("_GetHash_<C>", "_BoolArray_GetHash_<C>");
    bool_array_vtable.AddInterface("IComparable");
    bool_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(bool_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // ObjectArray: wrapper around std::vector<void*>
  {
    VirtualTable object_array_vtable("ObjectArray", sizeof(ObjectDescriptor) + sizeof(std::vector<void*>));
    object_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    object_array_vtable.AddFunction("_destructor_<M>", "_ObjectArray_destructor_<M>");
    object_array_vtable.AddFunction("_Equals_<C>_IComparable", "_ObjectArray_Equals_<C>_IComparable");
    object_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_ObjectArray_IsLess_<C>_IComparable");
    object_array_vtable.AddFunction("_GetHash_<C>", "_ObjectArray_GetHash_<C>");
    object_array_vtable.AddInterface("IComparable");
    object_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(object_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // StringArray: more typed ObjectArray
  {
    VirtualTable string_array_vtable("StringArray", sizeof(ObjectDescriptor) + sizeof(std::vector<void*>));
    string_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    string_array_vtable.AddFunction("_destructor_<M>", "_StringArray_destructor_<M>");
    string_array_vtable.AddFunction("_Equals_<C>_IComparable", "_StringArray_Equals_<C>_IComparable");
    string_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_StringArray_IsLess_<C>_IComparable");
    string_array_vtable.AddFunction("_GetHash_<C>", "_StringArray_GetHash_<C>");
    string_array_vtable.AddInterface("IComparable");
    string_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(string_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Pointer: unsafe pointer type
  {
    VirtualTable pointer_vtable("Pointer", sizeof(ObjectDescriptor) + sizeof(void*));
    pointer_vtable.AddField("Object", sizeof(ObjectDescriptor));
    pointer_vtable.AddFunction("_destructor_<M>", "_Pointer_destructor_<M>");
    pointer_vtable.AddFunction("_Equals_<C>_IComparable", "_Pointer_Equals_<C>_IComparable");
    pointer_vtable.AddFunction("_IsLess_<C>_IComparable", "_Pointer_IsLess_<C>_IComparable");
    pointer_vtable.AddFunction("_GetHash_<C>", "_Pointer_GetHash_<C>");
    pointer_vtable.AddInterface("IComparable");
    pointer_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(pointer_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // PointerArray: unsafe array of pointers
  {
    VirtualTable pointer_array_vtable("PointerArray", sizeof(ObjectDescriptor) + sizeof(std::vector<void*>));
    pointer_array_vtable.AddField("Object", sizeof(ObjectDescriptor));
    pointer_array_vtable.AddFunction("_destructor_<M>", "_PointerArray_destructor_<M>");
    pointer_array_vtable.AddFunction("_Equals_<C>_IComparable", "_PointerArray_Equals_<C>_IComparable");
    pointer_array_vtable.AddFunction("_IsLess_<C>_IComparable", "_PointerArray_IsLess_<C>_IComparable");
    pointer_array_vtable.AddFunction("_GetHash_<C>", "_PointerArray_GetHash_<C>");
    pointer_array_vtable.AddInterface("IComparable");
    pointer_array_vtable.AddInterface("IHashable");
    auto result = repository.Add(std::move(pointer_array_vtable));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  return {};
}

} // namespace ovum::vm::runtime

namespace ovum::vm::execution_tree {

std::expected<void, std::runtime_error> RegisterBuiltinFunctions(FunctionRepository& repository) {
  using namespace ovum::vm::runtime;
  using namespace ovum::vm::execution_tree;

  // Int methods
  {
    auto function = CreateMethodFunction("_Int_int", 2, IntConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_Int", 2, IntCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_destructor_<M>", 1, IntDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_Equals_<C>_IComparable", 2, IntEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_IsLess_<C>_IComparable", 2, IntIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_ToString_<C>", 1, IntToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Int_GetHash_<C>", 1, IntGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Float methods
  {
    auto function = CreateMethodFunction("_Float_float", 2, FloatConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_Float", 2, FloatCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_destructor_<M>", 1, FloatDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_Equals_<C>_IComparable", 2, FloatEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_IsLess_<C>_IComparable", 2, FloatIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_ToString_<C>", 1, FloatToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Float_GetHash_<C>", 1, FloatGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Char methods
  {
    auto function = CreateMethodFunction("_Char_char", 2, CharConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_Char", 2, CharCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_destructor_<M>", 1, CharDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_Equals_<C>_IComparable", 2, CharEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_IsLess_<C>_IComparable", 2, CharIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_ToString_<C>", 1, CharToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Char_GetHash_<C>", 1, CharGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Byte methods
  {
    auto function = CreateMethodFunction("_Byte_byte", 2, ByteConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_Byte", 2, ByteCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_destructor_<M>", 1, ByteDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_Equals_<C>_IComparable", 2, ByteEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_IsLess_<C>_IComparable", 2, ByteIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_ToString_<C>", 1, ByteToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Byte_GetHash_<C>", 1, ByteGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Bool methods
  {
    auto function = CreateMethodFunction("_Bool_bool", 2, BoolConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_Bool", 2, BoolCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_destructor_<M>", 1, BoolDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_Equals_<C>_IComparable", 2, BoolEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_IsLess_<C>_IComparable", 2, BoolIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_ToString_<C>", 1, BoolToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Bool_GetHash_<C>", 1, BoolGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Nullable methods
  {
    auto function = CreateMethodFunction("_Nullable_Object", 2, NullableConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Nullable_destructor_<M>", 1, NullableDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // String methods
  {
    auto function = CreateMethodFunction("_String_String", 2, StringCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_destructor_<M>", 1, StringDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_Equals_<C>_IComparable", 2, StringEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_IsLess_<C>_IComparable", 2, StringIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_ToString_<C>", 1, StringToString);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_GetHash_<C>", 1, StringGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_Length_<C>", 1, StringLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_String_ToUtf8Bytes_<C>", 1, StringToUtf8Bytes);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // IntArray methods
  {
    auto function = CreateMethodFunction("_IntArray_int_int", 3, IntArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_IntArray", 2, IntArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_destructor_<M>", 1, IntArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Equals_<C>_IComparable", 2, IntArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_IsLess_<C>_IComparable", 2, IntArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Length_<C>", 1, IntArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_GetHash_<C>", 1, IntArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Clear_<M>", 1, IntArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_ShrinkToFit_<M>", 1, IntArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Reserve_<M>_int", 2, IntArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Capacity_<C>", 1, IntArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_Add_<M>_int", 2, IntArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_RemoveAt_<M>_int", 2, IntArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_InsertAt_<M>_int_int", 3, IntArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_SetAt_<M>_int_int", 3, IntArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_IntArray_GetAt_<C>_int", 2, IntArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // FloatArray methods
  {
    auto function = CreateMethodFunction("_FloatArray_int_float", 3, FloatArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_FloatArray", 2, FloatArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_destructor_<M>", 1, FloatArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Equals_<C>_IComparable", 2, FloatArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_IsLess_<C>_IComparable", 2, FloatArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Length_<C>", 1, FloatArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_GetHash_<C>", 1, FloatArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Clear_<M>", 1, FloatArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_ShrinkToFit_<M>", 1, FloatArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Reserve_<M>_int", 2, FloatArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Capacity_<C>", 1, FloatArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_Add_<M>_float", 2, FloatArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_RemoveAt_<M>_int", 2, FloatArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_InsertAt_<M>_int_float", 3, FloatArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_SetAt_<M>_int_float", 3, FloatArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_FloatArray_GetAt_<C>_int", 2, FloatArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // CharArray methods
  {
    auto function = CreateMethodFunction("_CharArray_int_char", 3, CharArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_CharArray", 2, CharArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_destructor_<M>", 1, CharArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Equals_<C>_IComparable", 2, CharArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_IsLess_<C>_IComparable", 2, CharArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Length_<C>", 1, CharArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_GetHash_<C>", 1, CharArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Clear_<M>", 1, CharArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_ShrinkToFit_<M>", 1, CharArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Reserve_<M>_int", 2, CharArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Capacity_<C>", 1, CharArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_Add_<M>_char", 2, CharArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_RemoveAt_<M>_int", 2, CharArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_InsertAt_<M>_int_char", 3, CharArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_SetAt_<M>_int_char", 3, CharArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_CharArray_GetAt_<C>_int", 2, CharArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // ByteArray methods
  {
    auto function = CreateMethodFunction("_ByteArray_int_byte", 3, ByteArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_ByteArray", 2, ByteArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_destructor_<M>", 1, ByteArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Equals_<C>_IComparable", 2, ByteArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_IsLess_<C>_IComparable", 2, ByteArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Length_<C>", 1, ByteArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_GetHash_<C>", 1, ByteArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Clear_<M>", 1, ByteArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_ShrinkToFit_<M>", 1, ByteArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Reserve_<M>_int", 2, ByteArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Capacity_<C>", 1, ByteArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_Add_<M>_byte", 2, ByteArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_RemoveAt_<M>_int", 2, ByteArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_InsertAt_<M>_int_byte", 3, ByteArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_SetAt_<M>_int_byte", 3, ByteArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_GetAt_<C>_int", 2, ByteArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // ByteArray view casting constructors
  {
    auto function = CreateMethodFunction("_ByteArray_IntArray", 2, ByteArrayFromIntArray);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_FloatArray", 2, ByteArrayFromFloatArray);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_CharArray", 2, ByteArrayFromCharArray);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ByteArray_BoolArray", 2, ByteArrayFromBoolArray);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // BoolArray methods
  {
    auto function = CreateMethodFunction("_BoolArray_int_bool", 3, BoolArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_BoolArray", 2, BoolArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_destructor_<M>", 1, BoolArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Equals_<C>_IComparable", 2, BoolArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_IsLess_<C>_IComparable", 2, BoolArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Length_<C>", 1, BoolArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_GetHash_<C>", 1, BoolArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Clear_<M>", 1, BoolArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_ShrinkToFit_<M>", 1, BoolArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Reserve_<M>_int", 2, BoolArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Capacity_<C>", 1, BoolArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_Add_<M>_bool", 2, BoolArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_RemoveAt_<M>_int", 2, BoolArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_InsertAt_<M>_int_bool", 3, BoolArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_SetAt_<M>_int_bool", 3, BoolArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_BoolArray_GetAt_<C>_int", 2, BoolArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // ObjectArray methods
  {
    auto function = CreateMethodFunction("_ObjectArray_int_Object", 3, ObjectArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_ObjectArray", 2, ObjectArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_destructor_<M>", 1, ObjectArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Equals_<C>_IComparable", 2, ObjectArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_IsLess_<C>_IComparable", 2, ObjectArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Length_<C>", 1, ObjectArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_GetHash_<C>", 1, ObjectArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Clear_<M>", 1, ObjectArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_ShrinkToFit_<M>", 1, ObjectArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Reserve_<M>_int", 2, ObjectArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Capacity_<C>", 1, ObjectArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_Add_<M>_Object", 2, ObjectArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_RemoveAt_<M>_int", 2, ObjectArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_InsertAt_<M>_<C>_int_Object", 3, ObjectArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_SetAt_<M>_int_Object", 3, ObjectArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_ObjectArray_GetAt_<C>_int", 2, ObjectArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // StringArray methods
  {
    auto function = CreateMethodFunction("_StringArray_int_String", 3, StringArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_StringArray", 2, StringArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_destructor_<M>", 1, StringArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Equals_<C>_IComparable", 2, StringArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_IsLess_<C>_IComparable", 2, StringArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Length_<C>", 1, StringArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_GetHash_<C>", 1, StringArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Clear_<M>", 1, StringArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_ShrinkToFit_<M>", 1, StringArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Reserve_<M>_int", 2, StringArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Capacity_<C>", 1, StringArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_Add_<M>_String", 2, StringArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_RemoveAt_<M>_int", 2, StringArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_InsertAt_<M>_int_String", 3, StringArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_SetAt_<M>_int_String", 3, StringArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_StringArray_GetAt_<C>_int", 2, StringArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // PointerArray methods
  {
    auto function = CreateMethodFunction("_PointerArray_int_Pointer", 3, PointerArrayConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_PointerArray", 2, PointerArrayCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_destructor_<M>", 1, PointerArrayDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Equals_<C>_IComparable", 2, PointerArrayEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_IsLess_<C>_IComparable", 2, PointerArrayIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // Pointer methods
  {
    auto function = CreateMethodFunction("_Pointer_pointer", 2, PointerConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Pointer_Pointer", 2, PointerCopyConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Pointer_destructor_<M>", 1, PointerDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Pointer_Equals_<C>_IComparable", 2, PointerEquals);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Pointer_IsLess_<C>_IComparable", 2, PointerIsLess);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_Pointer_GetHash_<C>", 1, PointerGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Length_<C>", 1, PointerArrayLength);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_GetHash_<C>", 1, PointerArrayGetHash);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Clear_<M>", 1, PointerArrayClear);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_ShrinkToFit_<M>", 1, PointerArrayShrinkToFit);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Reserve_<M>_int", 2, PointerArrayReserve);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Capacity_<C>", 1, PointerArrayCapacity);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_Add_<M>_Pointer", 2, PointerArrayAdd);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_RemoveAt_<M>_int", 2, PointerArrayRemoveAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_InsertAt_<M>_int_Pointer", 3, PointerArrayInsertAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_SetAt_<M>_int_Pointer", 3, PointerArraySetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_PointerArray_GetAt_<C>_int", 2, PointerArrayGetAt);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  // File methods
  {
    auto function = CreateMethodFunction("_File", 1, FileConstructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_destructor_<M>", 1, FileDestructor);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Open_<M>_String_String", 3, FileOpen);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Close_<M>", 1, FileClose);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_IsOpen_<C>", 1, FileIsOpen);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Read_<M>_Int", 2, FileRead);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Write_<M>_ByteArray", 2, FileWrite);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_ReadLine_<M>", 1, FileReadLine);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_WriteLine_<M>_String", 2, FileWriteLine);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Seek_<M>_Int", 2, FileSeek);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Tell_<C>", 1, FileTell);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  {
    auto function = CreateMethodFunction("_File_Eof_<C>", 1, FileEof);
    auto result = repository.Add(std::move(function));
    if (!result.has_value()) {
      return std::unexpected(result.error());
    }
  }

  return {};
}

} // namespace ovum::vm::execution_tree
