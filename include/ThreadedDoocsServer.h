#pragma once

#include <random>
#include <string>
#include <thread>

#include <eq_svr.h>

extern int eq_server(int, char**);

/**
 * @brief Helper class to spawn a DOOCS-server in-process
 *
 * ThreadedDoocsServer is a class that provides a simple DOOCS server
 * running in a separate thread with a random RPC number for potential
 * parallel test execution
 */
class ThreadedDoocsServer {
 public:
  /**
   * @brief Creates a new ThreadedDoocsServer, optionally automatically starting it
   * @param configFile name of the server configuration file
   * @param argc number of elements in argv
   * @param argv array of C strings to pass as command line args to the server
   * @param autoStart whether or not not automatically start the server
   *
   * If autoStart is `false`, `argc` should be `0` and argv should be `nullptr`.
   */
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

  /**
   * @brief Start the thread manually.
   * @param argc number of elements in argv
   * @param argv array of C strings to pass as command line args to the server
   *
   * Only use if the object was created with `autoStart=false`
   */
  void start(int argc, char* argv[]) {
    // start the server if not already started
    if(not _doocsServerThread.joinable()) _doocsServerThread = std::thread{eq_server, argc, argv};
  }

  virtual ~ThreadedDoocsServer() {
    eq_exit();
    _doocsServerThread.join();
  }

  /**
   * @brief Get the RPC number used by this server
   * @return the random RPC number for the server
   */
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
