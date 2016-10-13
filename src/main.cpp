
#include "MenuSystem.hpp"
#include "RecordManager.hpp"
//#include <onions-common/Log.hpp>
//#include <onions-common/Utils.hpp>
#include <botan/botan.h>
//#include <fstream>

Botan::LibraryInitializer init("thread_safe");

int main(int argc, char** argv)
{
  auto manager = std::make_shared<RecordManager>();
  MenuSystem menu(manager);
  menu.mainMenu();
}

/*
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

//

void manageRecord(uint8_t, short);
uint8_t countAvailableCPUs();

int main(int argc, char** argv)
{
  char* keyPath = 0;
  bool license = false;
  short port = 9050;
  uint8_t workers = countAvailableCPUs();

    struct poptOption po[] = {
        {
            "hsKey", 'k', POPT_ARG_STRING, &keyPath, 11001,
            "Specifies a path to a HS private key", "<path>",
        },
        {"port", 'p', POPT_ARG_SHORT, &port, 0,
         "SOCKS port to use for anonymous communication.", "<port>"},
        {"license", 'l', POPT_ARG_NONE, &license, 11002,
         "Print software license and exit", NULL},
        {"workers", 'w', POPT_ARG_SHORT, &workers, 0,
         "Number of worker threads to use to make a Record valid.", "<1 -
    255>"},
        POPT_AUTOHELP{NULL, 0, 0, NULL, 0, NULL, NULL}};

    if (!Utils::parse(
            poptGetContext(NULL, argc, const_cast<const char**>(argv), po, 0)))
    {
      std::cout << "Failed to parse command-line arguments. Aborting.\n";
      return EXIT_FAILURE;
    }

    if (license)
    {
      std::cout << "Modified/New BSD License" << std::endl;
      return EXIT_SUCCESS;
    }

    if (keyPath)
    {
      HS::setKeyPath(keyPath);
      if (!Utils::loadKey(keyPath))  // test if it can be loaded
        return EXIT_FAILURE;
    }
    else
    {
      std::cerr << "Missing path to HS key! Specify with -k or --hsKey!\n";
      return EXIT_FAILURE;
    }

    manageRecord(workers, port);

  return EXIT_SUCCESS;
}


void manageRecord(uint8_t workers, short port)
{
  std::ifstream recordFile;
  std::string saveFile = Utils::getWorkingDirectory() + "record.save";
  recordFile.open(saveFile, std::fstream::in);
  if (recordFile.is_open())
  {
    Log::get().notice("Loading cached Record from disk...");

    Json::Value obj;
    recordFile >> obj;
    // HS::sendRecord(Common::parseRecord(obj), port);
  }
  else
  {
    auto r = HS::createRecord(workers);

    Log::get().notice("Caching Record to disk...");
    std::fstream recordOut(saveFile, std::fstream::out);
    recordOut << r->asJSON();
    recordOut.close();

    HS::sendRecord(r, port);
  }
}


uint8_t countAvailableCPUs()
{
  std::ifstream cpuinfo;
  cpuinfo.open("/proc/cpuinfo", std::fstream::in);
  if (!cpuinfo.is_open())
  {
    std::cerr << "Cannot open /proc/cpuinfo" << std::endl;
    exit(EXIT_FAILURE);
  }

  // grep -c ^processor /proc/cpuinfo
  uint8_t cpus = 0;
  std::string line;
  while (std::getline(cpuinfo, line))
    if (Utils::strBeginsWith(line, "processor"))
      cpus++;

  return cpus;
}
  */
