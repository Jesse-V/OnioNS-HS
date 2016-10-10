
#ifndef RECORD_MANAGER_HPP
#define RECORD_MANAGER_HPP

#include <onions-common/records/Record.hpp>
#include <botan/auto_rng.h>

class RecordManager
{
 public:
  static RecordManager& get()
  {
    static RecordManager instance;
    return instance;
  }

  void mainMenu() const;
  void registerName() const;
  void modifyName() const;
  void printHelp() const;
  int showMenu() const;

 private:
  RecordManager() {}
  RecordManager(RecordManager const&) = delete;
  void operator=(RecordManager const&) = delete;

  RecordPtr generateRecord() const;
  void addOnionServiceKey(RecordPtr&) const;
  void makeValid(RecordPtr&, const EdDSA_KEY&, int) const;

  EdDSA_KEY generateSecretKey() const;
  std::vector<std::string> getWordList() const;
  std::vector<std::string> toWords(const std::vector<std::string>&,
                                   const EdDSA_KEY&) const;
  int readInt(const std::string&,
              const std::string&,
              std::function<bool(int)>) const;
  std::string readMultiLineUntil(const std::string&,
                                 std::function<bool(const std::string&)>) const;

  void printParagraphs(std::vector<std::string>& text) const;
};

#endif
