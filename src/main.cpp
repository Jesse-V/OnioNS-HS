
#include "HS.hpp"
#include <onions-common/Utils.hpp>
#include <botan/botan.h>
#include <iostream>

Botan::LibraryInitializer init("thread_safe");

int main(int argc, char** argv)
{
  char* keyPath = NULL, logPath = NULL;
  bool license = false;

  struct poptOption po[] = {{
                             "hsKey",
                             'k',
                             POPT_ARG_STRING,
                             &keyPath,
                             11001,
                             "Specifies a path to a HS private key",
                             "<path>",
                            },
                            {
                             "license",
                             'l',
                             POPT_ARG_NONE,
                             &license,
                             11002,
                             "Print software license and exit",
                            },
                            POPT_AUTOHELP{NULL}};

  bool b = Utils::parse(
      argc, poptGetContext(NULL, argc, const_cast<const char**>(argv), po, 0));

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
  HS::sendRecord(r);

  return EXIT_SUCCESS;
}
