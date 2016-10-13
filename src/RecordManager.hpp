
#ifndef RECORD_MANAGER_HPP
#define RECORD_MANAGER_HPP

#include "Terminal.hpp"
#include <onions-common/records/Record.hpp>
#include <mutex>
#include <atomic>

class RecordManager
{
 public:
  RecordManager();
  ~RecordManager();

  void registerName();
  void modifyName();

 private:
  struct WorkerData
  {
    size_t id;
    uint32_t nonce = 0;
    uint32_t bestNonce = 0;
    double bestWeight = 0;
    std::vector<uint8_t> edScope;
    RSA_SIGNATURE rsaSig;
  };

  typedef std::shared_ptr<WorkerData> WorkerDataPtr;

  void generateRecord();
  void addOnionServiceKey();
  void signRecord();
  void startWorkers(size_t);
  void makeValid(const WorkerDataPtr&);
  void showWorkerStatus(const std::vector<WorkerDataPtr>&, size_t);

  void generateSecretKey();
  std::vector<std::string> getWordList() const;
  std::vector<std::string> edKey2Words(const std::vector<std::string>&) const;
  std::string readMultiLineUntil(const std::string&,
                                 std::function<bool(const std::string&)>) const;
  std::string getElapsedTime(const std::chrono::steady_clock::time_point&);

  std::shared_ptr<Botan::RSA_PrivateKey> rsaSecKey_;
  EdDSA_KEY edSecKey_, edPubKey_;
  RecordPtr record_;
  std::atomic<bool> timeRemaining_;
  std::mutex mutex_;
};

#endif
