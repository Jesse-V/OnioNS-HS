
#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <iostream>

class Terminal
{
 public:
  static void printLine(const std::string&);
  static void clearLine();

  enum Font : int
  {
    // styling
    BOLD = 1,
    FAINT = 2,
    STANDOUT = 3,
    UNDERLINE = 4,
    BLINK = 5,
    REVERSE = 7,
    HIDDEN = 8,

    // undo styling
    NO_BOLD = 21,
    NO_FAINT = 22,
    NO_STANDOUT = 23,
    NO_UNDERLINE = 24,
    NO_BLINK = 25,
    NO_REVERSE = 27,

    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    DEFAULT = 39,
  };

  enum Modifier : int
  {
    BACKGROUND = 10,
    LIGHT = 60
  };

  class Style
  {  // https://stackoverflow.com/questions/2616906

   public:
    Style(Font pCode) : _code(pCode) {}
    Style(Font pCode, Modifier mod) : _code(pCode + mod) {}
    friend std::ostream& operator<<(std::ostream& os, const Style& mod)
    {
      return os << "\033[" << mod._code << "m";
    }

   private:
    int _code;
  };
};

#endif
