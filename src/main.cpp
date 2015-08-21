
#include "HS.hpp"
#include <onions-common/Utils.hpp>
#include <botan/botan.h>
#include <iostream>

// Botan::LibraryInitializer init("thread_safe");

int main(int argc, char** argv)
{
  char* keyPath = 0;
  bool license = false;
  short port = 9050;

  struct poptOption po[] = {{
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
                             "SOCKS port to use for anonymizing claims on "
                             "domain names. The default is 9050.",
                             "<port>"},
                            {"license",
                             'l',
                             POPT_ARG_NONE,
                             &license,
                             11002,
                             "Print software license and exit",
                             NULL},
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

  auto r = HS::createRecord();
  HS::sendRecord(r, port);

  return EXIT_SUCCESS;
}
