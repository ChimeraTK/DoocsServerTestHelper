#pragma once

#include <random>
#include <string>
#include <thread>
#include <memory>
#include <doocs/Server.h>

#include "doocsServerTestHelper.h"

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem.hpp>

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
    assert(not _configFile.empty());

    auto pos = _configFile.find(".conf");
    _serverName = _configFile.substr(0, pos);

    // rename the instance to include the rpcNo, so we don't have conflicts with config files when running in parallel
    _serverNameInstance = _serverName + "_" + rpcNo();
    _configFileInstance = _serverNameInstance + ".conf";

    // instance is renamed by changing argv[0] - need to copy the whole argv array so we don't modify it for the caller!
    _argv.resize(argc);
    _serverNameInstanceC = std::shared_ptr<char[]>(new char[_serverNameInstance.size() + 1]);
    strcpy(_serverNameInstanceC.get(), _serverNameInstance.c_str());
    _argv[0] = _serverNameInstanceC.get();
    for(int i = 1; i < argc; ++i) {
      _argv[i] = strdup(argv[i]);
    }

    // create locks to avoid accidental conflicts with concurrent instances of the same or other tests
    _rpcNoLockFile = "/var/lock/rpcNo_" + rpcNo() + ".lock";
    _bpnLockFile = "/var/lock/bpn_" + bpn() + ".lock";
    std::ofstream(_configFileInstance, std::ofstream::out);
    std::ofstream(_rpcNoLockFile, std::ofstream::out);
    std::ofstream(_bpnLockFile, std::ofstream::out);
    _configMutex = boost::interprocess::file_lock(_configFileInstance.c_str());
    _rpcNoMutex = boost::interprocess::file_lock(_rpcNoLockFile.c_str());
    _bpnMutex = boost::interprocess::file_lock(_bpnLockFile.c_str());
    _configLock = std::unique_lock<boost::interprocess::file_lock>(_configMutex);
    _rpcNoLock = std::unique_lock<boost::interprocess::file_lock>(_rpcNoMutex);
    _bpnLock = std::unique_lock<boost::interprocess::file_lock>(_bpnMutex);

    // also need to have symlink for the executable with the new name
    boost::filesystem::create_symlink(_serverName, _serverNameInstance);

    // update config file with the RPC number and BPN
    std::string command = "sed " + _configFile + " -e 's/^SVR.RPC_NUMBER:.*$/SVR.RPC_NUMBER: " + rpcNo() +
        "/' -e 's/^SVR.BPN:.*$/SVR.BPN: " + bpn() + "/' > " + _configFileInstance;
    auto rc = std::system(command.c_str());
    (void)rc;
    assert(rc == 0);

    if(autoStart) start(_argv.size(), _argv.data());
  }

  void start(int argc, char* argv[]) {
    // start the server
    object_name = _serverName.c_str();
    // We have to register the wait_for_update() function of the DoocsServerTestHelper with the doocs server.
    // This is happening in DoocsServerTestHelper::initialise. We need an instance of the doocs::Server class for it.
    _doocsServer.reset(new doocs::Server(object_name, doocs::Server::CallsEqCreate::yes));
    DoocsServerTestHelper::initialise(_doocsServer.get());
    _doocsServerThread = std::thread([&]() { _doocsServer->run(argc, argv); });
  }

  virtual ~ThreadedDoocsServer() {
    DoocsServerTestHelper::shutdown(); // calls eq_exit() and releases the locks held by the test
    _doocsServerThread.join();
    for(int i = 1; i < _argv.size(); i++) {
      free(_argv[i]);
    }

    // cleanup files we have created
    boost::filesystem::remove(_configFileInstance);
    boost::filesystem::remove(_serverNameInstance);
    boost::filesystem::remove(_rpcNoLockFile);
    boost::filesystem::remove(_bpnLockFile);
  }

  std::string rpcNo() {
    if(_rpcNo.empty()) {
      std::random_device rd;
      std::uniform_int_distribution<int> dist(620000000, 999999999);
      _rpcNo = std::to_string(dist(rd));
    }

    return _rpcNo;
  }

  std::string bpn() {
    if(_bpn.empty()) {
      std::random_device rd;
      std::uniform_int_distribution<int> dist(2000, 6500);
      // range is 20000 to 65000 in steps of 10 (BPNs allocate a block of port numbers used for ZeroMQ)
      // avoid the "usual" range of 6000-10000, because there might be a normally configured server running on the
      // same machine using a BPN in that range...
      _bpn = std::to_string(dist(rd)) + "0";
    }

    return _bpn;
  }

 protected:
  std::shared_ptr<char[]> _serverNameInstanceC;
  std::vector<char*> _argv;
  std::string _serverName, _serverNameInstance;
  std::string _rpcNo, _bpn;
  std::string _configFile, _configFileInstance;
  std::string _rpcNoLockFile;
  std::string _bpnLockFile;
  boost::interprocess::file_lock _configMutex;
  boost::interprocess::file_lock _rpcNoMutex;
  boost::interprocess::file_lock _bpnMutex;
  std::unique_lock<boost::interprocess::file_lock> _configLock;
  std::unique_lock<boost::interprocess::file_lock> _rpcNoLock;
  std::unique_lock<boost::interprocess::file_lock> _bpnLock;
  std::thread _doocsServerThread;
  std::unique_ptr<doocs::Server> _doocsServer;
};
