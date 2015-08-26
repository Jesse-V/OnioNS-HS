
#include "HS.hpp"
#include <onions-common/Utils.hpp>
#include <botan/botan.h>
#include <fstream>
#include <iostream>

// Botan::LibraryInitializer init("thread_safe");

uint8_t countAvailableCPUs();

int main(int argc, char** argv)
{
  char* keyPath = 0;
  bool license = false;
  short port = 9050;
  uint8_t workers = countAvailableCPUs();

  struct poptOption po[] = {
      {
       "hsKey",
       'k',
       POPT_ARG_STRING,
       &keyPath,
       11001,
       "Specifies a path to a HS private key",
       "<path>",
      },
      {"port",
       'p',
       POPT_ARG_SHORT,
       &port,
       0,
       "SOCKS port to use for anonymous communication.",
       "<port>"},
      {"license",
       'l',
       POPT_ARG_NONE,
       &license,
       11002,
       "Print software license and exit",
       NULL},
      {"workers",
       'w',
       POPT_ARG_SHORT,
       &workers,
       0,
       "Number of worker threads to use to make a Record valid.",
       "<1 - 255>"},
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

  auto r = HS::createRecord(workers);
  HS::sendRecord(r, port);

  return EXIT_SUCCESS;
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
