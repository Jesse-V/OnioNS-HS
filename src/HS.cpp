
#include "HS.hpp"
#include <onions-common/containers/records/CreateR.hpp>
#include <onions-common/tcp/SocksClient.hpp>
#include <onions-common/Config.hpp>
#include <onions-common/Log.hpp>
#include <onions-common/Utils.hpp>
#include <iostream>

std::string HS::keyPath_;


RecordPtr HS::createRecord()
{
  auto r = promptForRecord();

  std::cout << "\n" << *r << "\n" << std::endl;
  std::cout << "Record prepared. Hit Enter to begin making it valid. ";
  std::string tmp;
  std::getline(std::cin, tmp);

  r->makeValid(4);

  std::cout << std::endl;
  std::cout << *r << std::endl;

  std::cout << std::endl;
  auto json = r->asJSON();
  std::cout << "Final Record is " << json.length()
            << " bytes, ready for transmission: \n\n" << json << std::endl;

  return r;
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
         " The association between these names and your hidden service is"
         " cryptographically locked and made valid after some computational"
         " work. This work will follow the prompts for the domains. \n";

  std::string name;
  std::cout << "\nThe primary domain name must end in \".tor\"" << std::endl;
  while (name.length() < 5 || !Utils::strEndsWith(name, ".tor"))
  {
    std::cout << "The primary domain name: ";
    std::getline(std::cin, name);
    name = Utils::trimString(name);
  }

  std::string pgp;
  std::cout << "\nYou may optionally supply your PGP fingerprint, \n"
               "which must be a power of two in length." << std::endl;
  while (!Utils::isPowerOfTwo(pgp.length()))
  {
    std::cout << "Your PGP fingerprint: ";
    std::getline(std::cin, pgp);  //"AD97364FC20BEC80"

    pgp = Utils::trimString(pgp);
    if (pgp.empty())
      break;
  }

  std::cout << "\nYou may provide up to 24 subdomain-destination pairs.\n"
               "These must end with \"." << name
            << "\". Leave the "
               "subdomain blank when finished." << std::endl;

  NameList list;
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
  auto r = std::make_shared<CreateR>(Utils::loadKey(keyPath_), name, pgp);
  r->setSubdomains(list);

  return r;
}



bool HS::sendRecord(const RecordPtr& r, short socksPort)
{
  auto addr = Config::getQuorumNode()[0];
  auto socks = SocksClient::getCircuitTo(
      addr["ip"].asString(), static_cast<short>(addr["port"].asInt()),
      socksPort);
  if (!socks)
    throw std::runtime_error("Unable to connect!");

  std::cout << "Uploading Record..." << std::endl;
  auto received = socks->sendReceive("upload", r->asJSON());
  if (received["type"].asString() == "error")
  {
    Log::get().error(received["value"].asString());
    return false;
  }

  std::cout << "Record upload complete; your claim has been accepted.\n";

  return true;
}



void HS::setKeyPath(const std::string& keyPath)
{
  keyPath_ = keyPath;
}
