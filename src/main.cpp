
#include "HS.hpp"
#include <onions-common/Flags.hpp>
#include <botan/botan.h>

Botan::LibraryInitializer init("thread_safe");

int main(int argc, char** argv)
{
  // if (!Flags::get().parse(argc, argv))
  //  return EXIT_FAILURE;

  HS::get().createRecord();

  return EXIT_SUCCESS;
}
