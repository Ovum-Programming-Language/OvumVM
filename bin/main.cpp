#include <iostream>
#include <string>
#include <vector>

#include "lib/vm_ui/vm_ui_functions.hpp"

int main(int32_t argc, char** argv) {
  std::vector<std::string> args = std::vector<std::string>(argv, argv + argc);

  const std::string sample = R"(
function:1 _Global_Main_StringArray {

    while {
        IntLessEqual
    } then {
        PrintLine
        IntAdd
    }

    Return
}
  )";

  return StartVmConsoleUI({"ovumc", sample}, std::cout, std::cin, std::cerr);
}
