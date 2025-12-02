#include <iostream>
#include <string>
#include <vector>

#include "lib/vm_ui/vm_ui_functions.hpp"

int main(int32_t argc, char** argv) {
  std::vector<std::string> args = std::vector<std::string>(argv, argv + argc);

  const std::string sample = R"(
function:1 _Global_Main_StringArray {
    PushInt 1
    SetLocal 1

    while {
        LoadLocal 1
        PushInt 5
        IntLessEqual
    } then {
        LoadLocal 1
        CallConstructor _Int_int
        Call _Int_ToString_<C>
        PrintLine
        LoadLocal 1
        PushInt 1
        IntAdd
        SetLocal 1
    }

    PushInt 0
    Return
}
  )";

  return StartVmConsoleUI({"ovumc", sample}, std::cout, std::cin, std::cerr);
}
