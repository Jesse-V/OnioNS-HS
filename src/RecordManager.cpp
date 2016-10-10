
#include "RecordManager.hpp"
#include "Terminal.hpp"
#include <onions-common/Utils.hpp>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
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

// ************************** MENU OPTIONS ************************** //

void RecordManager::mainMenu() const
{
  std::cout << '\n' << bold << cyan;
  std::cout << "Welcome to the OnioNS name management portal! \n";
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
    std::cout << "\n\n";
  }

  RecordPtr record = generateRecord();
  addOnionServiceKey(record);
  makeValid(record, edSecKey, 1);
}



void RecordManager::modifyName() const
{
  std::vector<std::string> text;

  text.push_back(std::string(
      "This functionality has not yet been implemented. Fortunately, the "
      "network is in Debug mode, so your records will not be copied into "
      "production. For the time being, feel free to register as many names as "
      "you like. There is no rate limiting in Debug mode."));

  printParagraphs(text);
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



int RecordManager::showMenu() const
{
  std::cout << blue << '\n';
  std::cout << "Main menu. Select from one of the options below:\n";
  std::cout << reset;
  std::cout << "1) Register a new name \n";
  std::cout << "2) Modify an existing name record \n";
  std::cout << "3) Get help and learn more \n";
  std::cout << "4) Close this application \n";
  std::cout << blue;

  return readInt("Your selection:", "That is not a valid selection.",
                 [](int i) { return i >= 0 && i <= 4; });
}


// ************************** RECORD SETUP ************************** //


RecordPtr RecordManager::generateRecord() const
{
  std::string type = "ticket";
  std::string name = "example.tor";
  std::string pgp = "AD97364FC20BEC80";
  StringMap subdomains;
  subdomains.push_back(std::make_pair("ddg", "duckduckgo.tor"));

  uint32_t rng = 1234567890, nonce = 0;
  return std::make_shared<Record>(type, name, pgp, subdomains, rng, nonce);
}



void RecordManager::addOnionServiceKey(RecordPtr& record) const
{
  std::vector<std::string> text;
  text.push_back(
      "Please paste the private key of your onion service. By default, this "
      "key is located at /var/lib/tor/hidden_service/private_key, although "
      "this may be different if you customized the path. The key will begin "
      "with \"-----BEGIN RSA PRIVATE KEY-----\". This will correspond to the "
      "destination for your registered name. We will never write the private "
      "key to disk. Please also note the value of the \"hostname\" file in the "
      "same directory.");
  printParagraphs(text);

  std::shared_ptr<Botan::RSA_PrivateKey> rsaSecKey;
  bool correctKey = true;
  do
  {
    std::string keyStr = readMultiLineUntil(
        "Paste your RSA onion service key: \n", [](const std::string& line) {
          return line == "-----END RSA PRIVATE KEY-----";
        });

    rsaSecKey = Utils::decodeRSA(keyStr);
    if (rsaSecKey->get_n().bits() != Const::RSA_KEY_LEN)
    {
      std::cerr << red << "\nThis is not a 1024-bit RSA key!\n" << reset;
      continue;
    }

    record->setServicePublicKey(rsaSecKey);
    std::cout << "\nThis key corresponds to " << record->computeOnion() << '\n';
    std::cout << "Is this correct? [y] ";

    std::string line;
    char c;
    getline(std::cin, line);
    std::istringstream(line) >> c;
    correctKey = (c == 'Y' || c == 'y');
    std::cout << '\n';

  } while (!correctKey);

  // sign the Record fields
  Botan::AutoSeeded_RNG rng;
  Botan::PK_Signer signer(*rsaSecKey, "EMSA-PSS(SHA-384)");
  auto scope = record->getServiceSigningScope();
  auto sigVector = signer.sign_message(scope, scope.size(), rng);

  // add the signature to the record
  RSA_SIGNATURE signature;
  std::copy(sigVector.begin(), sigVector.end(), signature.begin());
  record->setServiceSignature(signature);
}



void RecordManager::makeValid(RecordPtr& record,
                              const EdDSA_KEY& edSecKey,
                              int workers) const
{
  // get public key
  EdDSA_KEY edPubKey;
  ed25519_publickey(edSecKey.data(), edPubKey.data());
  record->setMasterPublicKey(edPubKey);

  // please start at 0 and increment for the PoW, it will help me research how
  // much work people are doing
  uint32_t nonce = 0;
  PoW_SCOPE powScope;
  std::copy(edPubKey.begin(), edPubKey.end(), powScope.begin());
  uint8_t* powNonce = powScope.data() + powScope.size() - sizeof(uint32_t);

  std::vector<uint8_t> edScope = record->asBytes(false);
  uint8_t* edNonce = edScope.data() + edScope.size() - sizeof(uint32_t);

  using namespace std::chrono;
  auto st = steady_clock::now();
  uint32_t min = UINT32_MAX;
  uint32_t max = 0;

  std::cout << "Threshold: " << Const::POW_WORD_0 << '\n';

  while (true)
  {
    nonce++;
    memcpy(powNonce, &nonce, sizeof(uint32_t));
    memcpy(edNonce, &nonce, sizeof(uint32_t));

    ed25519_sign(edScope.data(), edScope.size(), edSecKey.data(),
                 edPubKey.data(), powScope.data() + Const::EdDSA_KEY_LEN);
    uint32_t val = Record::computePOW(powScope);

    if (val < min)
      min = val;
    if (val > max)
      max = val;

    if (nonce % 100000 == 0)
    {
      auto diff = duration_cast<milliseconds>(steady_clock::now() - st).count();
      float seconds = diff / 1000.f;

      Terminal::clearLine();
      std::cout << seconds << " seconds, " << (nonce / seconds)
                << " i/second. Max = " << max << ", min = " << min;
      std::cout.flush();
    }
  }
  /*
    EdDSA_SIG edSig;
    ed25519_sign(edScope.data(), edScope.size(), edSecKey.data(),
    edPubKey.data(),
                 edSig.data());
    record->setMasterSignature(edSig);
    record->setNonce(nonce);

    std::cout << *record << '\n';

    std::cout << "Threshold: " << std::to_string(Const::POW_WORD_0) << '\n';
    return record; */
}


// ************************** UTILITIES ************************** //


EdDSA_KEY RecordManager::generateSecretKey() const
{
  static Botan::AutoSeeded_RNG rng;

  EdDSA_KEY key;
  rng.randomize(key.data(), Const::EdDSA_KEY_LEN);
  return key;
}



std::vector<std::string> RecordManager::toWords(
    const std::vector<std::string>& wordList,
    const EdDSA_KEY& sKey) const
{
  std::vector<std::string> results;
  if (sKey.size() != Const::EdDSA_KEY_LEN)
    return results;

  for (size_t j = 0; j < Const::EdDSA_KEY_LEN; j += 2)
  {
    uint16_t value = *reinterpret_cast<const uint16_t*>(sKey.data() + j);
    results.push_back(wordList[value]);
  }

  return results;
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
        std::cout << red << failMessage << '\n';
    }
    else
      std::cout << red << "Please provide a numeric value.\n";
  }
  std::cout << '\n';

  return choice;
}



std::string RecordManager::readMultiLineUntil(
    const std::string& prompt,
    std::function<bool(const std::string&)> check) const
{
  std::cout << blue << prompt << " " << reset;
  std::string result;
  std::string line;

  do
  {
    getline(std::cin, line);
    result += line + "\n";
  } while (!check(line));

  return result;
}



void RecordManager::printParagraphs(std::vector<std::string>& text) const
{
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


/*
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
