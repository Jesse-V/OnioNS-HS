
#include "RecordManager.hpp"
#include "Terminal.hpp"
//#include <onions-common/Config.hpp>
//#include <onions-common/Utils.hpp>
#include <botan/auto_rng.h>
#include <functional>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>


const size_t WORD_32 = 2 << 16;
const int LINE_LENGTH = 80;

const Terminal::Style bold(Terminal::BOLD);
const Terminal::Style no_bold(Terminal::NO_BOLD);
const Terminal::Style cyan(Terminal::CYAN, Terminal::LIGHT);
const Terminal::Style red(Terminal::RED, Terminal::LIGHT);
// const Terminal::Style green(Terminal::GREEN, Terminal::LIGHT);
// const Terminal::Style yellow(Terminal::YELLOW, Terminal::LIGHT);
const Terminal::Style blue(Terminal::BLUE, Terminal::LIGHT);
const Terminal::Style reset(Terminal::DEFAULT);


#ifndef INSTALL_PREFIX
#error CMake has not defined INSTALL_PREFIX!
#endif


void RecordManager::mainMenu() const
{
  std::cout << std::endl << bold << cyan;
  std::cout << "Welcome to the OnioNS name management portal!" << std::endl;
  std::cout << reset << no_bold << blue;

  while (true)
  {
    switch (showMenu())
    {
      case 1:
        registerName();
        break;

      case 2:
        modifyName();
        break;

      case 3:
        printHelp();
        break;

      case 4:
        exit(EXIT_SUCCESS);
    }
  }
}



RecordPtr generateRecord()
{
  std::string type = "Create";
  std::string name = "example.tor";
  std::string pgp = "1234";
  StringMap subdomains;
  uint32_t rng = 0, nonce = 0;
  return std::make_shared<Record>(type, name, pgp, subdomains, rng, nonce);
}



void RecordManager::registerName() const
{
  EdDSA_KEY edSecKey = generateSecretKey();
  auto wordList = getWordList();
  auto list = toWords(wordList, edSecKey);

  const int PRINT_SIZE = 4;
  for (int j = 0; j < PRINT_SIZE; j++)
  {
    std::cout << "  ";
    for (int k = 0; k < PRINT_SIZE; k++)
      std::cout << std::left << std::setw(19) << list[j * PRINT_SIZE + k];
    std::cout << std::endl << std::endl;
  }



  RecordPtr record = generateRecord();
  std::cout << *record << std::endl;
  // record is missing edKey, serviceKey, serviceSignature, edSig

  /*
  std::string resolve(const std::string&) const;
    Botan::SecureVector<uint8_t> hash() const;
    uint32_t computePOW(const std::vector<uint8_t>&) const;
    bool computeValidity() const;
    std::string computeOnion() const;
    std::vector<uint8_t> asBytes(bool forSigning = false) const;
    Json::Value asJSON() const;
    friend std::ostream& operator<<(std::ostream&, const Record&);
    */



  /*
    Botan::AutoSeeded_RNG rng;
    Botan::PK_Signer signer(*privateKey_, "EMSA4(SHA-512)");
    auto bytes = r.getServiceSigningScope();
    auto sig = signer.sign_message(bytes, jsonStr.size(), rng);
    std::copy(sig.begin(), sig.end(), signature_.begin());
  */

  // ed25519_sign(message, message_len, sk, pk, signature);

  /*
  std::string bytes = asBytes(); // last 4 bytes is the nonce
  bytes.data()[size - 4]++;
  setSignature(sign(privateKey, bytes.data()))
  auto val = computePOW();
  if (val < best)
    bestNonce = nonce;
*/
}



void RecordManager::modifyName() const
{
  std::cout << "This functionality has not yet been implemented." << std::endl;
}



void RecordManager::printHelp() const
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

  text.push_back(std::string(
      "Online documentation is coming soon. We assume that this software will "
      "be run offline or on a headless environment, so we will continue to "
      "provide the necessary documentation within this software package."));

  printParagraphs(text);
}



int RecordManager::showMenu() const
{
  std::cout << blue << std::endl;
  std::cout << "Main menu. Select from one of the options below:\n";
  std::cout << reset;
  std::cout << "1) Register a new name \n";
  std::cout << "2) Modify an existing name record \n";
  std::cout << "3) Get help and learn more \n";
  std::cout << "4) Close this application \n";
  std::cout << blue;

  return readInt("Your selection:", "That is not a valid selection.",
                 [](int i) { return i >= 0 && i <= 4; });
  ;
}



std::vector<std::string> RecordManager::toWords(
    const std::vector<std::string>& wordList,
    const EdDSA_KEY& sKey) const
{
  std::vector<std::string> results;
  if (sKey.size() != Const::EdDSA_KEY_LEN)
    return results;

  for (int j = 0; j < Const::EdDSA_KEY_LEN; j += 2)
  {
    uint16_t value = *reinterpret_cast<const uint16_t*>(sKey.data() + j);
    results.push_back(wordList[value]);
  }

  return results;
}



EdDSA_KEY RecordManager::generateSecretKey() const
{
  static Botan::AutoSeeded_RNG rng;
  EdDSA_KEY key;
  rng.randomize(key.data(), Const::EdDSA_KEY_LEN);
  return key;
}



std::vector<std::string> RecordManager::getWordList() const
{
  std::ifstream file;
  std::string path = INSTALL_PREFIX + "/share/onions-hs/american-english-small";
  file.open(path, std::fstream::in);

  std::vector<std::string> results;
  if (file.is_open())
  {
    std::string line;
    while (file.good())
    {
      getline(file, line);
      results.push_back(line);
    }
    results.pop_back();

    for (int j = -7000; results.size() < WORD_32; j++)
      results.push_back(std::to_string(j));

    file.close();
  }
  else
    std::cerr << "Failed to open " << path << std::endl;

  return results;
}



int RecordManager::readInt(const std::string& prompt,
                           const std::string& failMessage,
                           std::function<bool(int)> check) const
{
  int choice = 0;
  std::string line;
  while (true)
  {
    Terminal::clearLine();
    std::cout << blue << prompt << " " << reset;
    getline(std::cin, line);
    if (std::istringstream(line) >> choice)
    {
      if (check(choice))
        break;
      else
        std::cout << red << failMessage << std::endl;
    }
    else
      std::cout << red << "Please provide a numeric value." << std::endl;
  }
  std::cout << std::endl;

  return choice;
}



void RecordManager::printParagraphs(std::vector<std::string>& text) const
{
  // print each paragraph, wrap a LINE_LENGTH chars
  std::for_each(text.begin(), text.end(), [](std::string& paragraph) {
    int index = LINE_LENGTH;
    while (index < paragraph.size())
    {
      auto pos = paragraph.find_last_of(' ', index);
      paragraph.replace(pos, 1, 1, '\n');
      index = pos + LINE_LENGTH;
    }
    std::cout << paragraph << std::endl << std::endl;
  });
}


/*
for (int j = 999999; j > 0; j--)
{
  printLine("Going " + std::to_string(j));
  clearLine();
  // std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

RecordPtr HS::promptForRecord()
{
std::cout
    << "Here you can claim a .tor domain and multiple subdomains for your"
       " hidden service. They can point to either a .tor or a .onion domain,"
       " keeping it all within Tor. For example, you may claim"
       " \"example.tor\" -> \"onions55e7yam27n.onion\", \"foo.example.tor\""
       " -> \"foo.tor\", \"bar.example.tor\" -> \"bar.tor\", and"
       " \"a.b.c.example.tor\" -> \"3g2upl4pq6kufc4m.onion\"."

std::string name;
std::cout << "\nThe primary domain name must end in \".tor\"" << std::endl;
while (name.length() < 5 || !Utils::strEndsWith(name, ".tor"))
{
  std::cout << "The primary domain name: ";
  std::getline(std::cin, name);
  name = Utils::trimString(name);
}

std::string pgp;
  std::cout << "Your PGP fingerprint: ";
  std::getline(std::cin, pgp);  //"AD97364FC20BEC80"

  pgp = Utils::trimString(pgp);
  // if (pgp.empty())
  //  break;
}

std::cout << "\nYou may provide up to 24 subdomain-destination pairs.\n"
             "These must end with \"."
          << name << "\". Leave the "
                     "subdomain blank when finished."
          << std::endl;

StringMap list;
for (int n = 0; n < 24; n++)
{
  std::string src = name, dest;

  while (!Utils::strEndsWith(src, "." + name))
  {
    std::cout << "Subdomain " << (n + 1) << ": ";
    std::getline(std::cin, src);
    src = Utils::trimString(src);

    if (src.length() == 0)
      break;
  }

  if (src.length() == 0)
    break;

  while ((!Utils::strEndsWith(dest, ".tor") &&
          !Utils::strEndsWith(dest, ".onion")) ||
         (Utils::strEndsWith(dest, ".onion") && dest.length() != 16 + 6))
  {
    std::cout << "   Destination: ";
    std::getline(std::cin, dest);
    dest = Utils::trimString(dest);
  }

  src.erase(src.find("." + name));
  list.push_back(std::make_pair(src, dest));
  std::cout << std::endl;
}

std::cout << std::endl;
// auto r = std::make_shared<CreateR>(Utils::loadKey(keyPath_), name, pgp);
// r->setSubdomains(list);

// return r;
return nullptr;
}
*/
