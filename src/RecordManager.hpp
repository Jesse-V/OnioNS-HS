
#ifndef RECORD_MANAGER_HPP
#define RECORD_MANAGER_HPP

#include <onions-common/records/Record.hpp>
#include <botan/rsa.h>

const size_t EdDSA_KEY_LEN = 32;
typedef std::shared_ptr<std::array<uint8_t, EdDSA_KEY_LEN>> EdDSA_KEY;

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

  // static RecordPtr createRecord(uint8_t workers);
  // static RecordPtr promptForRecord();
  // static bool sendRecord(const RecordPtr&, short socksPort);
  // static void setKeyPath(const std::string&);

  // private:
  //  static std::string keyPath_;

 private:
  RecordManager(){};
  RecordManager(RecordManager const&) = delete;
  void operator=(RecordManager const&) = delete;

  EdDSA_KEY generateSecretKey() const;
  std::vector<std::string> getWordList() const;
  std::vector<std::string> toWords(const std::vector<std::string>&,
                                   const EdDSA_KEY&) const;
  int readInt(const std::string&,
              const std::string&,
              std::function<bool(int)>) const;
  void printParagraphs(std::vector<std::string>& text) const;
};



#endif
