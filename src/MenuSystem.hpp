
#ifndef MENU_SYSTEM_HPP
#define MENU_SYSTEM_HPP

#include "RecordManager.hpp"
#include <memory>

class MenuSystem
{
 public:
  MenuSystem(const std::shared_ptr<RecordManager>&);

  void mainMenu() const;
  void printHelp() const;
  int showMenu() const;

 private:
  void printParagraphs(std::vector<std::string>& text) const;
  int readInt(const std::string&,
              const std::string&,
              std::function<bool(int)>) const;

  std::shared_ptr<RecordManager> manager_;
};

#endif
