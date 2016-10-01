
#include "Terminal.hpp"


void Terminal::printLine(const std::string& line)
{
  std::cout << line;
  std::cout.flush();
}

void Terminal::clearLine()
{
  std::cout << "\33[2K\r";  // VT100 escape codes
}
