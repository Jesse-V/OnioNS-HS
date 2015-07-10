
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
  static void setKeyPath(const std::string&);

 private:
  static std::string keyPath_;
};

#endif
