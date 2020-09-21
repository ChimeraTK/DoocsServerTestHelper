#pragma once

#include <random>
#include <string>
#include <thread>

#include <eq_svr.h>
#include <eq_fct.h>

extern int eq_server(int, char**);

#ifndef object_name
extern const char* object_name;
#endif

class ThreadedDoocsServer {
 public:
  ThreadedDoocsServer(std::string configFile, int argc, char* argv[], bool autoStart = true)
  : _serverName{"ThreadedDoocsServer"}, _configFile(std::move(configFile)) {
    if(not _configFile.empty()) {
      auto pos = _configFile.find(".conf");
      _serverName = _configFile.substr(0, pos);
      // update config file with the RPC number
      std::string command = "sed -i " + _configFile + " -e 's/^SVR.RPC_NUMBER:.*$/SVR.RPC_NUMBER: " + rpcNo() + "/'";
      auto rc = std::system(command.c_str());
      (void)rc;
    }

    if(autoStart) start(argc, argv);
  }

  void start(int argc, char* argv[]) {
    // start the server
    object_name = _serverName.c_str();
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
  std::string _serverName;
  std::string _rpcNo;
  std::string _configFile;
  std::thread _doocsServerThread;
};
