
#ifndef HIDDEN_SERVICE_HPP
#define HIDDEN_SERVICE_HPP

#include <onions-common/containers/records/Record.hpp>
#include <botan/rsa.h>

class HS
{
 public:
  static RecordPtr createRecord();
  static RecordPtr promptForRecord();
  static bool sendRecord(const RecordPtr&);

 private:
  static Botan::RSA_PrivateKey* loadKey();
};

#endif
