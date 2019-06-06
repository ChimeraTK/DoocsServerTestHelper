#pragma once

#include <random>
#include <string>
#include <thread>

#include <eq_svr.h>
#include <eq_fct.h>

extern int eq_server(int, char**);

class ThreadedDoocsServer {
 public:
  ThreadedDoocsServer(std::string configFile, int argc, char* argv[], bool autoStart = true)
  : _configFile(std::move(configFile)) {
    if(not _configFile.empty()) {
      // update config file with the RPC number
      std::string command = "sed -i " + _configFile +
          " "
          "-e 's/^SVR.RPC_NUMBER:.*$/SVR.RPC_NUMBER: " +
          rpcNo() + "/'";
      auto rc = std::system(command.c_str());
      (void)rc;
    }

    if(autoStart) start(argc, argv);
  }

  void start(int argc, char* argv[]) {
    // start the server
    _doocsServerThread = std::thread{eq_server, argc, argv};
  }

  virtual ~ThreadedDoocsServer() {
    eq_exit();
    _doocsServerThread.join();
  }

  std::string rpcNo() {
    if(_rpcNo.empty()) {
      std::random_device rd;
      std::uniform_int_distribution<int> dist(620000000, 999999999);
      _rpcNo = std::to_string(dist(rd));
    }

    return _rpcNo;
  }

 protected:
  std::string _rpcNo;
  std::string _configFile;
  std::thread _doocsServerThread;
};
