#pragma once

#include "doocsServerTestHelper.h"

#include <doocs/Server.h>

#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <eq_fct.h>

#include <memory>
#include <random>
#include <string>
#include <thread>

class ThreadedDoocsServer {
 public:
  ThreadedDoocsServer(std::string configFile, int argc, char* argv[], std::unique_ptr<doocs::Server> doocsServer,
      bool autoStart = true);

  void start();

  virtual ~ThreadedDoocsServer();

  std::string rpcNo();

  std::string bpn();

 protected:
  std::mutex _mx_serverInfo;
  std::shared_ptr<char[]> _serverNameInstanceC;
  std::vector<char*> _argv{};
  std::string _serverName{}, _serverNameInstance{};
  std::string _rpcNo{}, _bpn{};
  std::string _configFile{}, _configFileInstance{};
  std::string _rpcNoLockFile{};
  std::string _bpnLockFile{};
  boost::interprocess::file_lock _configMutex;
  boost::interprocess::file_lock _rpcNoMutex;
  boost::interprocess::file_lock _bpnMutex;
  std::unique_lock<boost::interprocess::file_lock> _configLock;
  std::unique_lock<boost::interprocess::file_lock> _rpcNoLock;
  std::unique_lock<boost::interprocess::file_lock> _bpnLock;
  std::thread _doocsServerThread;
  std::unique_ptr<doocs::Server> _doocsServer;
};
