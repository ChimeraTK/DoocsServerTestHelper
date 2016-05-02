/*
 * doocsServerTestHelper.cc
 *
 *  Created on: May 5, 2016
 *      Author: Martin Hierholzer
 */

#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <eq_fct.h>

#include <doocsServerTestHelper.h>


/**********************************************************************************************************************/

// event number, defined in llrfCtrl_rpc_server.cc
extern uint64_t gen_event;

// pointer to server location
extern EqFctSvr *server_eq;

bool doocsServerTestHelper::allowUpdate = false;
bool doocsServerTestHelper::allowSigusr1 = false;
bool doocsServerTestHelper::interceptSystemCalls = false;
bool doocsServerTestHelper::doNotProcessSignalsInDoocs = false;
int doocsServerTestHelper::magic_sleep_time_sec = 0xDEAD;
int doocsServerTestHelper::magic_sleep_time_usec = 0xBEEF;
int doocsServerTestHelper::event = 0;
std::mutex doocsServerTestHelper::update_mutex;
std::mutex doocsServerTestHelper::sigusr1_mutex;


/**********************************************************************************************************************/

extern "C" int nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining) {

    // call original-equivalent version if requested time does not match the magic signature
    if(!doocsServerTestHelper::interceptSystemCalls					  || 
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

void doocsServerTestHelper::initialise(bool _doNotProcessSignalsInDoocs) {

    // acquire locks
    sigusr1_mutex.lock();
    update_mutex.lock();
    
    // enable intercepting system calls
    interceptSystemCalls = true;
    doNotProcessSignalsInDoocs = _doNotProcessSignalsInDoocs;

    // trigger sigusr1 once to make sure the server has started
    runSigusr1();

    // change update rate, so nanosleep is detected properly
    server_eq->SetSrvUpdateRate(magic_sleep_time_sec,magic_sleep_time_usec);
}

/**********************************************************************************************************************/

void doocsServerTestHelper::runSigusr1() {
    event++;    // increase event number
    gen_event = event;
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


