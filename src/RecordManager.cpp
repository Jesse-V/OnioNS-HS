
#include "RecordManager.hpp"
#include <onions-common/Utils.hpp>
#include <botan/rsa.h>
#include <botan/pubkey.h>
#include <functional>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <sstream>

#ifndef INSTALL_PREFIX
#error CMake has not defined INSTALL_PREFIX!
#endif

const Terminal::Style RED(Terminal::RED, Terminal::LIGHT);
const Terminal::Style BLUE(Terminal::BLUE, Terminal::LIGHT);
const Terminal::Style RESET_TERM(Terminal::DEFAULT);


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
  /*
  std::vector<std::string> text;

  text.push_back(std::string(
      "This functionality has not yet been implemented. Fortunately, the "
      "network is in Debug mode, so your records will not be copied into "
      "production. For the time being, feel free to register as many names as "
      "you like. There is no rate limiting in Debug mode."));

  printParagraphs(text);
  */
}



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
  /*
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
*/

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
      std::cerr << RED << "\nThis is not a 1024-bit RSA key!\n" << RESET_TERM;
      continue;
    }

    record->setServicePublicKey(rsaSecKey);
    std::cout << "\nThis key corresponds to " << record->computeOnion() << '\n';
    std::cout << "Is this correct? [y] ";

    std::string line;
    char c;
    getline(std::cin, line);
    std::istringstream(line) >> c;
    correctKey = (c == 'Y' || c == 'y' || line == "");
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

  uint32_t nonce = 0;
  PoW_SCOPE powScope;
  std::copy(edPubKey.begin(), edPubKey.end(), powScope.begin());
  uint8_t* powNonce = powScope.data() + powScope.size() - sizeof(uint32_t);

  std::vector<uint8_t> edScope = record->asBytes(false);
  uint8_t* edNonce = edScope.data() + edScope.size() - sizeof(uint32_t);

  uint32_t bestNonce = 0;
  double bestWeight = 0;

  std::cout << "In this next step, we will use your master key to repeatedly "
               "sign your record.\nWe need to generate a valid record with a "
               "high Significance value.\nThe higher the Significance value, "
               "the higher the chance that the OnioNS network\nwill approve "
               "your registration. This may take some time...\n\n";

  using namespace std::chrono;
  auto st = steady_clock::now();

  while (true)
  {
    nonce++;
    memcpy(powNonce, &nonce, sizeof(uint32_t));
    memcpy(edNonce, &nonce, sizeof(uint32_t));

    ed25519_sign(edScope.data(), edScope.size(), edSecKey.data(),
                 edPubKey.data(), powScope.data() + Const::EdDSA_KEY_LEN);

    if (Record::computePOW(powScope) < Const::PoW_THRESHOLD)
    {  // below threshold, thus qualifying

      double weight = Record::computeWeight(powScope);
      if (weight > bestWeight)
      {
        bestWeight = weight;
        bestNonce = nonce;

        Terminal::clearLine();
        std::cout << "Iteration " << nonce << " found better weight " << weight
                  << std::endl;

        // update record
        EdDSA_SIG edSig;
        ed25519_sign(edScope.data(), edScope.size(), edSecKey.data(),
                     edPubKey.data(), edSig.data());
        record->setMasterSignature(edSig);
        record->setNonce(nonce);
      }
    }
    else if (nonce % 100000 == 0)
    {
      auto diff = duration_cast<milliseconds>(steady_clock::now() - st).count();
      float seconds = diff / 1000.f;

      Terminal::clearLine();
      std::cout << seconds << " seconds elapsed, iteration " << nonce << ", "
                << (nonce / seconds) << " i/sec";
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

    const size_t WORDLIST_SIZE = 2 << 16;
    for (int j = -7000; results.size() < WORDLIST_SIZE; j++)
      results.push_back(std::to_string(j));

    file.close();
  }
  else
    std::cerr << "Failed to open " << path << std::endl;

  return results;
}



std::string RecordManager::readMultiLineUntil(
    const std::string& prompt,
    std::function<bool(const std::string&)> check) const
{
  std::cout << BLUE << prompt << " " << RESET_TERM;
  std::string result;
  std::string line;

  do
  {
    getline(std::cin, line);
    result += line + "\n";
  } while (!check(line));

  return result;
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
