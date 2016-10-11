
#ifndef RECORD_MANAGER_HPP
#define RECORD_MANAGER_HPP

#include "Terminal.hpp"
#include <onions-common/records/Record.hpp>

class RecordManager
{
 public:
  void registerName() const;
  void modifyName() const;

  RecordPtr generateRecord() const;
  void addOnionServiceKey(RecordPtr&) const;
  void makeValid(RecordPtr&, const EdDSA_KEY&, int) const;

 private:
  EdDSA_KEY generateSecretKey() const;
  std::vector<std::string> getWordList() const;
  std::vector<std::string> toWords(const std::vector<std::string>&,
                                   const EdDSA_KEY&) const;
  std::string readMultiLineUntil(const std::string&,
                                 std::function<bool(const std::string&)>) const;
};

#endif
