
#ifndef HIDDEN_SERVICE_HPP
#define HIDDEN_SERVICE_HPP

#include <onions-common/records/Record.hpp>
#include <botan/rsa.h>

class HS
{
 public:
  static RecordPtr createRecord(uint8_t workers);
  static RecordPtr promptForRecord();
  static bool sendRecord(const RecordPtr&, short socksPort);
  static void setKeyPath(const std::string&);

 private:
  static std::string keyPath_;
};

#endif
