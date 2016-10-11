
#include "MenuSystem.hpp"
#include <sstream>

const Terminal::Style BOLD(Terminal::BOLD);
const Terminal::Style NO_BOLD(Terminal::NO_BOLD);
const Terminal::Style CYAN(Terminal::CYAN, Terminal::LIGHT);
const Terminal::Style BLUE(Terminal::BLUE, Terminal::LIGHT);
const Terminal::Style RED(Terminal::RED, Terminal::LIGHT);
const Terminal::Style RESET_TERM(Terminal::DEFAULT);


MenuSystem::MenuSystem(const std::shared_ptr<RecordManager>& manager)
    : manager_(manager)
{
}



int MenuSystem::showMenu() const
{
  std::cout << BLUE << '\n';
  std::cout << "Main menu. Select from one of the options below:\n";
  std::cout << RESET_TERM;
  std::cout << "1) Register a new name \n";
  std::cout << "2) Modify an existing name record \n";
  std::cout << "3) Get help and learn more \n";
  std::cout << "4) Close this application \n";
  std::cout << BLUE;

  return readInt("Your selection:", "That is not a valid selection.",
                 [](int i) { return i >= 0 && i <= 4; });
}



void MenuSystem::mainMenu() const
{
  std::cout << '\n' << BOLD << CYAN;
  std::cout << "Welcome to the OnioNS name management portal! \n";
  std::cout << RESET_TERM << NO_BOLD << BLUE;

  while (true)
  {
    switch (showMenu())
    {
      case 1:
        manager_->registerName();
        break;

      case 2:
        manager_->modifyName();
        break;

      case 3:
        printHelp();
        break;

      case 4:
        exit(EXIT_SUCCESS);
    }
  }
}



void MenuSystem::printHelp() const
{
  // clang-format should hard-wrap and insert quotes into the below text
  std::vector<std::string> text;

  text.push_back(std::string(
      "The Onion Name System (OnioNS) is a privacy-enhanced, metadata-free, "
      "and highly-usable DNS for Tor onion services. Using this software, you "
      "can anonymously register a meaningful and globally-unique domain name "
      "for your service. OnioNS is built on top of the Tor network, does not "
      "require any modifications to the Tor binary, and there are no central "
      "authorities in charge of the domain names. This project was "
      "specifically engineered to solve the usability problem with onion "
      "services."));

  text.push_back(std::string(
      "We have designed the Onion Name System (OnioNS) with security and "
      "privacy in mind. Unlike the Internet DNS, OnioNS does not ask for any "
      "personal information when you register a name for your service. "
      "Instead, we use a combination of asymmetric cryptographic keys to map "
      "your onion/hidden service's RSA key to a meaningful name. Specifically, "
      "your RSA key digitally signs a master edDSA key. The master key in turn "
      "signs the RSA key, a meaningful name, subdomains, and other data. This "
      "way, everyone can verify that the name belongs to you."));

  text.push_back(std::string(
      "In effect, you use the master key to have complete control over your "
      "name record. Once you have registered a name on your own, you will be "
      "prompted to distribute this record to specific nodes inside the Tor "
      "network. If accepted by the network, users will be able to type the "
      "meaningful name into the Tor Browser and load your onion service. The "
      "digital signatures ensure authenticity and prevent anyone from "
      "manipulating your record without your permission."));

  text.push_back(std::string(
      "It is very important that you keep the master key safe. This portal "
      "will translate the master key into a series of words, which will make "
      "it easier for you to write down and retrieve. You can keep these words "
      "offline and air-gapped. This software will never write the master key "
      "to disk and will securely erase it from RAM when the program exits. "
      "We cannot recover lost keys, restore control of your record if the "
      "master key has been compromised, or hand your key over to any third "
      "party."));

  text.push_back(
      std::string("We have built OnioNS because we believe that onion services "
                  "have some very useful applications. It is our hope that "
                  "this software will make them easier to access and for you "
                  "to manage. We encourage you to use OnioNS on onion services "
                  "that reflect positively on Tor and its community."));

  text.push_back(std::string(
      "Online documentation is coming soon. We assume that this software will "
      "be run offline or on a headless environment, so we will continue to "
      "provide the necessary documentation within this software package."));

  printParagraphs(text);
}



void MenuSystem::printParagraphs(std::vector<std::string>& text) const
{
  const size_t LINE_LENGTH = 80;

  // print each paragraph, wrap a LINE_LENGTH chars
  std::for_each(text.begin(), text.end(), [](std::string& paragraph) {
    size_t index = LINE_LENGTH;
    while (index < paragraph.size())
    {
      auto pos = paragraph.find_last_of(' ', index);
      paragraph.replace(pos, 1, 1, '\n');
      index = pos + LINE_LENGTH;
    }
    std::cout << paragraph << "\n\n";
  });
}



int MenuSystem::readInt(const std::string& prompt,
                        const std::string& failMessage,
                        std::function<bool(int)> check) const
{
  int choice = 0;
  std::string line;
  while (true)
  {
    Terminal::clearLine();
    std::cout << BLUE << prompt << " " << RESET_TERM;
    getline(std::cin, line);
    if (std::istringstream(line) >> choice)
    {
      if (check(choice))
        break;
      else
        std::cout << RED << failMessage << '\n';
    }
    else
      std::cout << RED << "Please provide a numeric value.\n";
  }
  std::cout << '\n';

  return choice;
}
