#include <iostream>
#include <string>
#include <vector>

#include "lib/vm_ui/vm_ui_functions.hpp"

int main(int32_t argc, char** argv) {
  std::vector<std::string> args = std::vector<std::string>(argv, argv + argc);

  const std::string sample = R"(
init-static {
    PushFloat 3.14159
    SetStatic 0
}

vtable Rectangle {
    size: 24
    interfaces {
        IShape
    }
    methods {
        _GetArea_<C>: _Rectangle_GetArea_<C>
        _GetPerimeter_<C>: _Rectangle_GetPerimeter_<C>
    }
    vartable {
        Width: Object@8
        Height: Object@16
    }
}

vtable Circle {
    size: 16
    interfaces {
        IShape
    }
    methods {
        _GetArea_<C>: _Circle_GetArea_<C>
        _GetPerimeter_<C>: _Circle_GetPerimeter_<C>
    }
    vartable {
        Radius: Object@8
    }
}

function:3 _Rectangle_float_float {
    LoadLocal 1
    CallConstructor _Float_float
    LoadLocal 0
    SetField 0
    LoadLocal 2
    CallConstructor _Float_float
    LoadLocal 0
    SetField 1
    LoadLocal 0
    Return
}

function:1 _Rectangle_GetArea_<C> {
    LoadLocal 0
    GetField 1
    Unwrap
    LoadLocal 0
    GetField 0
    Unwrap
    FloatMultiply
    CallConstructor _Float_float
    Return
}

function:1 _Rectangle_GetPerimeter_<C> {
    LoadLocal 0
    GetField 1
    Unwrap
    LoadLocal 0
    GetField 0
    Unwrap
    FloatAdd
    PushFloat 2.0
    FloatMultiply
    CallConstructor _Float_float
    Return
}

function:2 _Circle_float {
    LoadLocal 1
    CallConstructor _Float_float
    LoadLocal 0
    SetField 0
    LoadLocal 0
    Return
}

function:1 _Circle_GetArea_<C> {
    LoadLocal 0
    GetField 0
    Unwrap
    LoadStatic 0
    FloatMultiply
    LoadLocal 0
    GetField 0
    Unwrap
    FloatMultiply
    CallConstructor _Float_float
    Return
}

function:1 _Circle_GetPerimeter_<C> {
    LoadStatic 0
    PushFloat 2.0
    FloatMultiply
    LoadLocal 0
    GetField 0
    Unwrap
    FloatMultiply
    CallConstructor _Float_float
    Return
}

function:1 _Global_ProcessShape_IShape {
    LoadLocal 0
    CallVirtual _GetArea_<C>
    SetLocal 1
    LoadLocal 0
    CallVirtual _GetPerimeter_<C>
    SetLocal 2
    LoadLocal 1
    Call _Float_ToString_<C>
    PushString "Area: "
    StringConcat
    PushString ", Perimeter: "
    Swap
    StringConcat
    LoadLocal 2
    Call _Float_ToString_<C>
    Swap
    StringConcat
    Return
}

function:1 _Global_Main_StringArray {
    PushFloat 3.0
    PushFloat 5.0
    CallConstructor _Rectangle_float_float
    SetLocal 1
    PushFloat 2.5
    CallConstructor _Circle_float
    SetLocal 2
    LoadLocal 1
    Call _Global_ProcessShape_IShape
    SetLocal 3
    LoadLocal 2
    Call _Global_ProcessShape_IShape
    SetLocal 4
    LoadLocal 3
    PrintLine
    LoadLocal 4
    PrintLine
    PushInt 0
    Return
}
  )";

  return StartVmConsoleUI({"ovumc", sample}, std::cout, std::cin, std::cerr);
}
