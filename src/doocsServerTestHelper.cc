/*
 * doocsServerTestHelper.cc
 *
 *  Created on: May 5, 2016
 *      Author: Martin Hierholzer
 */

#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <eq_fct.h>

#include <doocsServerTestHelper.h>

using mtca4u::doocsServerTestHelper;

/**********************************************************************************************************************/

// pointer to server location
extern EqFctSvr *server_eq;

/**********************************************************************************************************************/

extern "C" int nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining) {
    if(doocsServerTestHelper::bootingServer) {
      doocsServerTestHelper::bootingServer = false;
      doocsServerTestHelper::setServerIsStarted.set_value();
    }

    // call original-equivalent version if requested time does not match the magic signature
    if(!doocsServerTestHelper::interceptSystemCalls                                   ||
        __requested_time->tv_sec  != doocsServerTestHelper::magic_sleep_time_sec      ||
       __requested_time->tv_nsec != doocsServerTestHelper::magic_sleep_time_usec*1000    ) {
      return clock_nanosleep(CLOCK_MONOTONIC,0,__requested_time,__remaining);
    }

    // magic signature found: wait until update requested via mutex
    do {
      doocsServerTestHelper::update_mutex.unlock();
      usleep(1);
      doocsServerTestHelper::update_mutex.lock();
    } while(!doocsServerTestHelper::allowUpdate);
    doocsServerTestHelper::allowUpdate = false;
    return 0;

}

/**********************************************************************************************************************/

extern "C" int sigwait(__const sigset_t *__restrict __set, int *__restrict __sig) {

    // call original-equivalent version if SIGUSR1 is not in the set
    if(!doocsServerTestHelper::interceptSystemCalls || !sigismember(__set, SIGUSR1)) {
      if(!doocsServerTestHelper::doNotProcessSignalsInDoocs) {
        siginfo_t __siginfo;
        int iret = sigwaitinfo(__set, &__siginfo);
        *__sig = __siginfo.si_signo;
        return iret;
      }
      else {
        siginfo_t __siginfo;
        sigset_t ___set;
        sigemptyset(&___set);
        int iret = sigwaitinfo(&___set, &__siginfo);
        *__sig = __siginfo.si_signo;
        return iret;
      }
    }

    // SIGUSR1 is in the set: wait until sigusr1 reqested via mutex
    do {
      doocsServerTestHelper::sigusr1_mutex.unlock();
      usleep(1);
      doocsServerTestHelper::sigusr1_mutex.lock();
    } while(!doocsServerTestHelper::allowSigusr1);
    doocsServerTestHelper::allowSigusr1 = false;

    // return a SIGUSR1
    *__sig = SIGUSR1;
    return 0;
}

/**********************************************************************************************************************/

namespace mtca4u {

  bool doocsServerTestHelper::allowUpdate = false;
  bool doocsServerTestHelper::allowSigusr1 = false;
  bool doocsServerTestHelper::interceptSystemCalls = false;
  bool doocsServerTestHelper::bootingServer = false;
  bool doocsServerTestHelper::doNotProcessSignalsInDoocs = false;
  int doocsServerTestHelper::magic_sleep_time_sec = 0xDEAD;
  int doocsServerTestHelper::magic_sleep_time_usec = 0xBEEF;
  std::mutex doocsServerTestHelper::update_mutex;
  std::mutex doocsServerTestHelper::sigusr1_mutex;
  std::promise<void> doocsServerTestHelper::setServerIsStarted;
  std::future<void> doocsServerTestHelper::serverIsStarted = doocsServerTestHelper::setServerIsStarted.get_future();

  /**********************************************************************************************************************/

  void doocsServerTestHelper::initialise(bool _doNotProcessSignalsInDoocs) {

      // acquire locks
      sigusr1_mutex.lock();
      update_mutex.lock();

      // wait until server initialised
      while(!server_eq) {
        usleep(10);
      }

      // enable intercepting system calls
      server_eq->lock();
      interceptSystemCalls = true;
      doNotProcessSignalsInDoocs = _doNotProcessSignalsInDoocs;
      bootingServer = true;
      server_eq->unlock();

      // wait until server properly started (i.e. nanosleep() called for the first time)
      doocsServerTestHelper::serverIsStarted.get();

      // change update rate, so nanosleep is detected properly
      server_eq->SetSrvUpdateRate(magic_sleep_time_sec,magic_sleep_time_usec);  // has no effect. why?
  }

  /**********************************************************************************************************************/

  void doocsServerTestHelper::runSigusr1() {
      allowSigusr1 = true;
      do {
        sigusr1_mutex.unlock();
        usleep(1);
        sigusr1_mutex.lock();
      } while(allowSigusr1);
  }

  /**********************************************************************************************************************/

  void doocsServerTestHelper::runUpdate() {
      allowUpdate = true;
      do {
        update_mutex.unlock();
        usleep(1);
        update_mutex.lock();
      } while(allowUpdate);
  }

  /**********************************************************************************************************************/

  void doocsServerTestHelper::shutdown() {
      kill(getpid(), SIGINT);
      usleep(100000);
      allowUpdate = true;
      allowSigusr1 = true;
      update_mutex.unlock();
      sigusr1_mutex.unlock();
  }

} /* namespace mtca4u */
