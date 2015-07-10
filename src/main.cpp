
#include "HS.hpp"
#include <onions-common/Utils.hpp>
#include <botan/botan.h>
#include <iostream>

Botan::LibraryInitializer init("thread_safe");

int main(int argc, char** argv)
{
  char* logPath;
  bool license = false;

  struct poptOption po[] = {{
                             "license",
                             'L',
                             POPT_ARG_NONE,
                             &license,
                             11002,
                             "Print software license and exit.",
                            },
                            POPT_AUTOHELP{NULL}};

  bool b = Utils::parse(
      argc, poptGetContext(NULL, argc, const_cast<const char**>(argv), po, 0));

  if (license)
  {
    std::cout << "Modified/New BSD License" << std::endl;
    return EXIT_SUCCESS;
  }

  HS::createRecord();

  return EXIT_SUCCESS;
}
