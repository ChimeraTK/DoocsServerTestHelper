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

/**********************************************************************************************************************/

// pointer to server location
extern EqFctSvr *server_eq;

/**********************************************************************************************************************/

extern "C" int nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining) {

    // call original-equivalent version if requested time does not match the magic signature
    if(__requested_time->tv_sec  != DoocsServerTestHelper::magic_sleep_time_sec      ||
       __requested_time->tv_nsec != DoocsServerTestHelper::magic_sleep_time_usec*1000    ) {
      return clock_nanosleep(CLOCK_MONOTONIC, 0, __requested_time, __remaining);
    }

    // if timing system is not yet initialised, sleep for 1 second
    if(!DoocsServerTestHelper::interceptSystemCalls) {
      struct timespec tv;
      tv.tv_sec = 1;
      tv.tv_nsec = 0;
      return clock_nanosleep(CLOCK_MONOTONIC, 0, &tv, __remaining);
    }

    // if this is the first call, acquire lock and set flag that the server has been started
    if(!DoocsServerTestHelper::serverStarted) {
      DoocsServerTestHelper::serverStarted = true;
      DoocsServerTestHelper::update_mutex.lock();
    }

    // magic signature found: wait until update requested via mutex
    do {
      DoocsServerTestHelper::update_mutex.unlock();
      usleep(1);
      DoocsServerTestHelper::update_mutex.lock();
    } while(!DoocsServerTestHelper::allowUpdate);
    DoocsServerTestHelper::allowUpdate = false;
    return 0;

}

/**********************************************************************************************************************/

extern "C" int sigwait(__const sigset_t *__restrict __set, int *__restrict __sig) {

    // call original-equivalent version if SIGUSR1 is not in the set
    if( !DoocsServerTestHelper::interceptSystemCalls || !DoocsServerTestHelper::serverStarted ||
        !sigismember(__set, SIGUSR1) ) {
      if(!DoocsServerTestHelper::doNotProcessSignalsInDoocs) {
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
      DoocsServerTestHelper::sigusr1_mutex.unlock();
      usleep(1);
      DoocsServerTestHelper::sigusr1_mutex.lock();
    } while(!DoocsServerTestHelper::allowSigusr1);
    DoocsServerTestHelper::allowSigusr1 = false;

    // return a SIGUSR1
    *__sig = SIGUSR1;
    return 0;
}

/**********************************************************************************************************************/

std::atomic<bool> DoocsServerTestHelper::allowUpdate(false);
std::atomic<bool> DoocsServerTestHelper::allowSigusr1(false);
std::atomic<bool> DoocsServerTestHelper::interceptSystemCalls(false);
std::atomic<bool> DoocsServerTestHelper::serverStarted(false);
std::atomic<bool> DoocsServerTestHelper::doNotProcessSignalsInDoocs(false);
const int DoocsServerTestHelper::magic_sleep_time_sec = 57005; // 0xDEAD
const int DoocsServerTestHelper::magic_sleep_time_usec = 48879; // 0xBEEF
std::mutex DoocsServerTestHelper::update_mutex;
std::mutex DoocsServerTestHelper::sigusr1_mutex;

/**********************************************************************************************************************/

void DoocsServerTestHelper::initialise(bool _doNotProcessSignalsInDoocs, bool useSigUsr1) {

    std::cout << "DoocsServerTestHelper::intialise(): Setting up virtual timing system..." << std::endl;

    // acquire locks
    sigusr1_mutex.lock();
    update_mutex.lock();

    // enable intercepting system calls
    allowUpdate = false;
    allowSigusr1 = false;
    interceptSystemCalls = true;
    doNotProcessSignalsInDoocs = _doNotProcessSignalsInDoocs;
    sigusr1_mutex.unlock();

    // wait until server properly started (i.e. nanosleep() called for the first time)
    do {
      update_mutex.unlock();
      usleep(1);
      update_mutex.lock();
    } while(!serverStarted);

    sigusr1_mutex.lock();

    // send ourselves SIGUSR1 to leave the sigwait we might have entered already in the sigusr1 thread
    if(useSigUsr1) {
      std::cout << "DoocsServerTestHelper::intialise(): Initialising the virtual interrupt_usr1 system..." << std::endl;
      kill(getpid(), SIGUSR1);
    }
    else {
      std::cout << "DoocsServerTestHelper::intialise(): Virtual interrupt_usr1 system is not initialised by request..." << std::endl;
    }

    // run update once to make sure everything is properly started
    runUpdate();

    std::cout << "DoocsServerTestHelper::intialise(): Initialisation complete!" << std::endl;

}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runSigusr1() {
    allowSigusr1 = true;
    do {
      sigusr1_mutex.unlock();
      usleep(1);
      sigusr1_mutex.lock();
    } while(allowSigusr1);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runUpdate() {
    allowUpdate = true;
    do {
      update_mutex.unlock();
      usleep(1);
      update_mutex.lock();
    } while(allowUpdate);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::shutdown() {
    kill(getpid(), SIGINT);
    usleep(100000);
    allowUpdate = true;
    allowSigusr1 = true;
    update_mutex.unlock();
    sigusr1_mutex.unlock();
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::doocsSetSpectrum( const std::string &name, const std::vector<float> &value ) {
    EqAdr ad;
    EqData ed, res;
    // fill spectrum data structure
    SPECTRUM spectrum;
    spectrum.d_spect_array.d_spect_array_len = value.size();
    spectrum.d_spect_array.d_spect_array_val = (float*) value.data();   // casts const-ness away, but will not be modified (hopefully)
    ed.set(&spectrum);
    // obtain location pointer
    ad.adr(name.c_str());
    EqFct *p = eq_get(&ad);
    ASSERT(p != NULL, std::string("Could not get location for property ")+name);
    // set spectrum
    p->lock();
    p->set(&ad,&ed,&res);
    p->unlock();
    // check for error
    ASSERT(res.error() == 0, std::string("Error writing spectrum property ")+name);
}

/**********************************************************************************************************************/

template<>
std::string DoocsServerTestHelper::doocsGet<std::string>( const char *name ) {
    EqAdr ad;
    EqData ed, res;
    // obtain location pointer
    ad.adr(name);
    EqFct *p = eq_get(&ad);
    ASSERT(p != NULL, std::string("Could not get location for property ")+name);
    // obtain value
    p->lock();
    p->get(&ad,&ed,&res);
    p->unlock();
    // check for errors
    ASSERT(res.error() == 0, std::string("Error reading property ")+name);
    // return requested type
    return res.get_string();
}
