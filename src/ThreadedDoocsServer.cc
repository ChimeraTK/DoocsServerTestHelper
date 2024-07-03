#include "ThreadedDoocsServer.h"

/*********************************************************************************************************************/

ThreadedDoocsServer::ThreadedDoocsServer(
    std::string configFile, int argc, char* argv[], std::unique_ptr<doocs::Server> doocsServer, bool autoStart)
: _configFile(std::move(configFile)), _doocsServer(std::move(doocsServer)) {
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
  std::ofstream give_me_a_name1(_configFileInstance, std::ofstream::out);
  std::ofstream give_me_a_name2(_rpcNoLockFile, std::ofstream::out);
  std::ofstream give_me_a_name3(_bpnLockFile, std::ofstream::out);
  _configMutex = boost::interprocess::file_lock(_configFileInstance.c_str());
  _rpcNoMutex = boost::interprocess::file_lock(_rpcNoLockFile.c_str());
  _bpnMutex = boost::interprocess::file_lock(_bpnLockFile.c_str());
  _configLock = std::unique_lock<boost::interprocess::file_lock>(_configMutex);
  _rpcNoLock = std::unique_lock<boost::interprocess::file_lock>(_rpcNoMutex);
  _bpnLock = std::unique_lock<boost::interprocess::file_lock>(_bpnMutex);

  // also need to have symlink for the executable with the new name
  boost::filesystem::create_symlink(_serverName, _serverNameInstance);

  // set directory name for history files
  std::string histdir = "hist_" + _serverNameInstance;
  setenv("HIST_DIR", histdir.c_str(), true);

  // update config file with the RPC number and BPN
  std::string command = "sed " + _configFile + " -e 's/^SVR.RPC_NUMBER:.*$/SVR.RPC_NUMBER: " + rpcNo() +
      "/' -e 's/^SVR.BPN:.*$/SVR.BPN: " + bpn() + "/' > " + _configFileInstance;
  auto rc = std::system(command.c_str());
  (void)rc;
  assert(rc == 0);

  // start server if autostart requested
  if(autoStart) {
    start();
  }
}

/*********************************************************************************************************************/

void ThreadedDoocsServer::start() {
  // We have to register the wait_for_update() function of the DoocsServerTestHelper with the doocs server.
  // This is happening in DoocsServerTestHelper::initialise. We need an instance of the doocs::Server class for it.
  DoocsServerTestHelper::initialise(_doocsServer.get());

  // Start the server in separate thread
  _doocsServerThread = std::thread([&]() { _doocsServer->run(_argv.size(), _argv.data()); });
}

/*********************************************************************************************************************/

ThreadedDoocsServer::~ThreadedDoocsServer() {
  DoocsServerTestHelper::shutdown(); // calls eq_exit() and releases the locks held by the test
  _doocsServerThread.join();
  for(size_t i = 1; i < _argv.size(); i++) {
    free(_argv[i]);
  }

  // cleanup files we have created
  boost::filesystem::remove(_configFileInstance);
  boost::filesystem::remove(_serverNameInstance);
  boost::filesystem::remove(_rpcNoLockFile);
  boost::filesystem::remove(_bpnLockFile);
}

/*********************************************************************************************************************/

std::string ThreadedDoocsServer::rpcNo() {
  std::lock_guard<std::mutex> lk(_mx_serverInfo);
  if(_rpcNo.empty()) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(620000000, 999999999);
    _rpcNo = std::to_string(dist(rd));
  }

  return _rpcNo;
}

/*********************************************************************************************************************/

std::string ThreadedDoocsServer::bpn() {
  std::lock_guard<std::mutex> lk(_mx_serverInfo);
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

/*********************************************************************************************************************/
