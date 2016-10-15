
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
#include <future>
#include <cmath>
#include <ctime>

#ifndef INSTALL_PREFIX
#error CMake has not defined INSTALL_PREFIX!
#endif

const Terminal::Style RED(Terminal::RED, Terminal::LIGHT);
const Terminal::Style BLUE(Terminal::BLUE, Terminal::LIGHT);
const Terminal::Style RESET_TERM(Terminal::DEFAULT);


RecordManager::RecordManager()
    : rsaSecKey_(nullptr), record_(nullptr), timeRemaining_(true)
{
  edSecKey_.fill(0);
  edPubKey_.fill(0);
}



RecordManager::~RecordManager()
{
  edSecKey_.fill(0);
  edPubKey_.fill(0);
}



void RecordManager::registerName()
{
  generateSecretKey();

  auto wordList = getWordList();
  auto list = edKey2Words(wordList);

  const int PRINT_SIZE = 4;
  for (int j = 0; j < PRINT_SIZE; j++)
  {
    std::cout << "  ";
    for (int k = 0; k < PRINT_SIZE; k++)
      std::cout << std::left << std::setw(19) << list[j * PRINT_SIZE + k];
    std::cout << "\n\n";
  }

  generateRecord();
  addOnionServiceKey();
  startWorkers(2);
}



void RecordManager::modifyName()
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



void RecordManager::generateRecord()
{
  std::string type = "ticket";
  std::string name = "example.tor";
  std::string pgp = "AD97364FC20BEC80";
  StringMap subdomains;
  subdomains.push_back(std::make_pair("ddg", "duckduckgo.tor"));

  uint32_t rng = 1234567890, nonce = 0;
  record_ = std::make_shared<Record>(type, name, pgp, subdomains, rng, nonce);
  record_->setMasterPublicKey(edPubKey_);
}



void RecordManager::addOnionServiceKey()
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

  bool correctKey = true;
  do
  {
    std::string keyStr = readMultiLineUntil(
        "Paste your RSA onion service key: \n", [](const std::string& line) {
          return line == "-----END RSA PRIVATE KEY-----";
        });

    rsaSecKey_ = Utils::decodeRSA(keyStr);
    if (rsaSecKey_->get_n().bits() != Const::RSA_KEY_LEN)
    {
      std::cerr << RED << "\nThis is not a 1024-bit RSA key!\n" << RESET_TERM;
      continue;
    }

    record_->setServicePublicKey(rsaSecKey_);
    std::cout << "\nThis key corresponds to " << record_->computeOnion();
    std::cout << "\nIs this correct? [y] ";

    std::string line;
    char c;
    getline(std::cin, line);
    std::istringstream(line) >> c;
    correctKey = (c == 'Y' || c == 'y' || line == "");
    std::cout << '\n';

  } while (!correctKey);

  signRecord();
}



RSA_SIGNATURE RecordManager::signRecord()
{
  // sign the Record fields
  static Botan::AutoSeeded_RNG rng;
  Botan::PK_Signer signer(*rsaSecKey_, "EMSA-PSS(SHA-384)");
  auto scope = record_->getServiceSigningScope();
  auto sigVector = signer.sign_message(scope, scope.size(), rng);

  // add the signature to the record
  RSA_SIGNATURE signature;
  std::copy(sigVector.begin(), sigVector.end(), signature.begin());
  return signature;
}



void RecordManager::makeValid(const WorkerDataPtr& wd)
{
  /*
    We are doing four things here:
      1) Resigning the record data so that the signature is valid
          The signature covers all record data except itself (of course)
      2) Checking that the proof-of-work is valid
          The PoW covers the edDSA key, edDSA signature, and the nonce
      3) Tracking the nonce that will generate the most weighted record
      4) Updating the RSA signature since a fast computer can max out the nonce

    The nonce is within the scope of the edDSA signature and the PoW, so we
    have to keep it updated twice.
  */

  // proof-of-work check covers edDSA key, edDSA signature, and nonce
  PoW_SCOPE powScope;
  std::copy(edPubKey_.begin(), edPubKey_.end(), powScope.begin());

  // get pointers to the nonces in each scope
  wd->edScope = record_->asBytes(false);
  uint8_t* edNonce = wd->edScope.data() + wd->edScope.size() - 4;
  uint8_t* powNonce = powScope.data() + powScope.size() - 4;

  while (timeRemaining_)
  {
    // re-sign record probabilistically
    // this will give us more entropy if the nonce is maxed
    mutex_.lock();
    wd->rsaSig = signRecord();
    wd->updatedRSA++;
    std::copy(wd->rsaSig.begin(), wd->rsaSig.end(), wd->edScope.begin());
    // std::cout << "\nWorker " << wd->id << " initialized."  << std::endl;
    mutex_.unlock();

    // main working loop
    for (wd->nonce = 0; timeRemaining_ && wd->nonce < UINT32_MAX; wd->nonce++)
    {
      // copy the nonce into each of the scopes
      memcpy(powNonce, &wd->nonce, sizeof(uint32_t));
      memcpy(edNonce, &wd->nonce, sizeof(uint32_t));

      // edDSA sign, place the signature in the PoW scope array
      ed25519_sign(wd->edScope.data(), wd->edScope.size(), edSecKey_.data(),
                   edPubKey_.data(), powScope.data() + Const::EdDSA_KEY_LEN);

      // check the proof-of-work
      if (Record::computePOW(powScope) <= Const::PoW_THRESHOLD)
      {  // below threshold, thus qualifying

        wd->nQualified++;

        // check the weight
        const double weight = Record::computeWeight(powScope);
        if (weight > wd->bestWeight)
        {
          wd->bestWeight = weight;
          wd->bestNonce = wd->nonce;
        }
      }
    }
  }

  mutex_.lock();
  std::cout << "\nWorker thread " << wd->id << " has finished.";
  mutex_.unlock();
}



void RecordManager::startWorkers(uint32_t nWorkers)
{
  /*
  std::cout << "In this next step, we will use your master key to repeatedly "
               "sign your record.\nWe need to generate a valid record with a "
               "high Significance value.\nThe higher the Significance value, "
               "the higher the chance that the OnioNS network\nwill approve "
               "your registration. This may take some time...\n\n";
*/

  std::vector<std::thread> workers;
  std::vector<std::shared_ptr<WorkerData>> wData;

  for (uint32_t j = 0; j < nWorkers; j++)
  {
    auto wd = std::make_shared<WorkerData>();
    wd->id = j + 1;
    wData.push_back(wd);
    workers.push_back(std::thread([wd, this]() { makeValid(wd); }));
  }

  std::cout << nWorkers << " threads active." << std::endl;
  std::for_each(workers.begin(), workers.end(),
                [](std::thread& t) { t.detach(); });

  showWorkerStatus(wData, nWorkers);
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  std::cout << std::endl;


  /*
    auto bestResult = std::max_element(wData.begin(), wData.end(),
                                        [](const std::shared_ptr<WorkerData>& a,
                                           const std::shared_ptr<WorkerData>& b)
    {
                                          return a->bestWeight < b->bestWeight;
                                        });*/

  std::shared_ptr<WorkerData> bestResult;
  double bestWeight = 0;
  for (uint32_t j = 0; j < nWorkers; j++)
  {
    if (wData[j]->bestWeight > bestWeight)
    {
      bestResult = wData[j];
      bestWeight = wData[j]->bestWeight;
    }
  }

  record_->setNonce(bestResult->bestNonce);
  record_->setServiceSignature(bestResult->rsaSig);
  EdDSA_SIG edSig;
  auto edScope = record_->asBytes(false);
  ed25519_sign(edScope.data(), edScope.size(), edSecKey_.data(),
               edPubKey_.data(), edSig.data());
  record_->setMasterSignature(edSig);
  std::cout << std::endl << *record_ << std::endl;

  /*
    // update record
    EdDSA_SIG edSig;
    ed25519_sign(edScope.data(), edScope.size(), edSecKey.data(),
                 edPubKey.data(), edSig.data());
    record->setServiceSignature(rsaSig);
    record->setMasterSignature(edSig);
    record->setNonce(nonce);

    std::cout << *record_ << std::endl;
  */
}



void RecordManager::showWorkerStatus(
    const std::vector<std::shared_ptr<WorkerData>>& wData,
    uint32_t nWorkers)
{
  const uint32_t SECONDS_ALLOWED = 25200;  // 7 = 25200, 9 = 32400
  const uint32_t UPDATE_DELAY = 2;
  double sumLast = 0;

  std::cout << "Elapsed Time | Current Speed | Average Speed | Total "
               "Iterations | Found | Reset | Best Weight"
            << std::endl;

  using namespace std::chrono;
  std::chrono::seconds interval(UPDATE_DELAY);
  steady_clock::time_point start = steady_clock::now();
  auto deadline = start;

  for (uint32_t j = 1; j <= SECONDS_ALLOWED / UPDATE_DELAY; j++)
  {
    deadline += interval;
    std::this_thread::sleep_until(deadline);

    double nonceSum = 0;
    uint32_t qualifySum = 0;
    uint32_t resetSum = 0;
    double bestWeight = 0;
    for (size_t w = 0; w < nWorkers; w++)
    {
      nonceSum += wData[w]->nonce + wData[w]->updatedRSA * UINT32_MAX;
      qualifySum += wData[w]->nQualified;
      resetSum += wData[w]->updatedRSA;
      if (wData[w]->bestWeight > bestWeight)
        bestWeight = wData[w]->bestWeight;
    }

    int current = static_cast<int>(round(nonceSum - sumLast) / UPDATE_DELAY);
    int avgSpeed = static_cast<int>(round(nonceSum / (j * UPDATE_DELAY)));
    sumLast = nonceSum;

    Terminal::clearLine();
    std::cout.precision(11);
    std::cout << "  " << std::setw(16) << getElapsedTime(start) << std::setw(16)
              << current << std::setw(17) << avgSpeed << std::setw(17)
              << nonceSum << std::setw(8) << qualifySum << std::setw(10)
              << resetSum << std::setw(9) << std::setprecision(4) << bestWeight;
    std::cout.flush();
  }

  timeRemaining_ = false;
}


// ************************** UTILITIES ************************** //


void RecordManager::generateSecretKey()
{
  static Botan::AutoSeeded_RNG rng;
  rng.randomize(edSecKey_.data(), Const::EdDSA_KEY_LEN);
  ed25519_publickey(edSecKey_.data(), edPubKey_.data());
}



std::vector<std::string> RecordManager::edKey2Words(
    const std::vector<std::string>& wordList) const
{
  std::vector<std::string> results;
  if (edSecKey_.size() != Const::EdDSA_KEY_LEN)
    return results;

  for (size_t j = 0; j < Const::EdDSA_KEY_LEN; j += 2)
  {
    uint16_t value = *reinterpret_cast<const uint16_t*>(edSecKey_.data() + j);
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



std::string RecordManager::getElapsedTime(
    const std::chrono::steady_clock::time_point& start)
{
  using namespace std::chrono;
  static char timeChars[32];

  system_clock::time_point tp_epoch;
  system_clock::time_point diff(steady_clock::now() - start);
  std::time_t tt = system_clock::to_time_t(diff);
  std::strftime(timeChars, sizeof(timeChars), "%H:%M:%S", std::gmtime(&tt));
  return std::string(timeChars);
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
