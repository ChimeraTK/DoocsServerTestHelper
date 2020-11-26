/*
 * doocsServerTestHelper.cc
 *
 *  Created on: May 5, 2016
 *      Author: Martin Hierholzer
 */

#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <eq_fct.h>

#include "doocsServerTestHelper.h"

/**********************************************************************************************************************/

// pointer to server location
extern EqFctSvr* server_eq;

/**********************************************************************************************************************/

extern "C" int nanosleep(__const struct timespec* __requested_time, struct timespec* __remaining) {
  // call original-equivalent version if requested time does not match the magic
  // signature
  if(__requested_time->tv_sec != DoocsServerTestHelper::magic_sleep_time_sec ||
      __requested_time->tv_nsec != DoocsServerTestHelper::magic_sleep_time_usec * 1000) {
    return clock_nanosleep(CLOCK_MONOTONIC, 0, __requested_time, __remaining);
  }

  // magic signature found: wait until update requested via mutex
  static thread_local std::unique_lock<std::mutex> update_lock{DoocsServerTestHelper::update_mutex};
  do {
    update_lock.unlock();
    usleep(1);
    update_lock.lock();
  } while(!DoocsServerTestHelper::allowUpdate);
  DoocsServerTestHelper::allowUpdate = false;
  return 0;
}

/**********************************************************************************************************************/

extern "C" int sigwait(__const sigset_t* __restrict __set, int* __restrict __sig) {
  // call original-equivalent version if SIGUSR1 is not in the set
  if(!sigismember(__set, SIGUSR1)) {
    // first wait on the specified signals
    siginfo_t __siginfo;
    int iret = sigwaitinfo(__set, &__siginfo);
    *__sig = __siginfo.si_signo;
    // if (in the mean time) doNotProcessSignalsInDoocs has been set by the test
    // code, wait on an empty set (-> forever)
    if(DoocsServerTestHelper::doNotProcessSignalsInDoocs) {
      siginfo_t __siginfo;
      sigset_t ___set;
      sigemptyset(&___set);
      iret = sigwaitinfo(&___set, &__siginfo);
      *__sig = __siginfo.si_signo;
    }
    return iret;
  }

  // SIGUSR1 is in the set: wait until sigusr1 reqested via mutex
  static thread_local std::unique_lock<std::mutex> sigusr1_lock{DoocsServerTestHelper::sigusr1_mutex};
  do {
    sigusr1_lock.unlock();
    usleep(1);
    sigusr1_lock.lock();
  } while(!DoocsServerTestHelper::allowSigusr1);
  DoocsServerTestHelper::allowSigusr1 = false;

  // return a SIGUSR1
  *__sig = SIGUSR1;
  return 0;
}

/**********************************************************************************************************************/

std::atomic<bool> DoocsServerTestHelper::allowUpdate(false);
std::atomic<bool> DoocsServerTestHelper::allowSigusr1(false);
std::atomic<bool> DoocsServerTestHelper::doNotProcessSignalsInDoocs(false);
const int DoocsServerTestHelper::magic_sleep_time_sec = 57005;  // 0xDEAD
const int DoocsServerTestHelper::magic_sleep_time_usec = 48879; // 0xBEEF
std::mutex DoocsServerTestHelper::update_mutex;
std::mutex DoocsServerTestHelper::sigusr1_mutex;
std::unique_lock<std::mutex> DoocsServerTestHelper::update_lock(DoocsServerTestHelper::update_mutex);
std::unique_lock<std::mutex> DoocsServerTestHelper::sigusr1_lock(DoocsServerTestHelper::sigusr1_mutex);

/**********************************************************************************************************************/

void DoocsServerTestHelper::setDoNotProcessSignalsInDoocs(bool _doNotProcessSignalsInDoocs) {
  doNotProcessSignalsInDoocs = _doNotProcessSignalsInDoocs;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runSigusr1() {
  allowSigusr1 = true;
  do {
    sigusr1_lock.unlock();
    usleep(1);
    sigusr1_lock.lock();
  } while(allowSigusr1);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runUpdate() {
  allowUpdate = true;
  do {
    update_lock.unlock();
    usleep(1);
    update_lock.lock();
  } while(allowUpdate);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::shutdown() {
  eq_exit();
  usleep(100000);
  allowUpdate = true;
  allowSigusr1 = true;
  update_lock.unlock();
  sigusr1_lock.unlock();
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::doocsSetSpectrum(const std::string& name, const std::vector<float>& value) {
  EqAdr ad;
  EqData ed, res;
  // fill spectrum data structure
  SPECTRUM spectrum;
  spectrum.comment.comment_len = 0;
  spectrum.comment.comment_val = nullptr;
  spectrum.tm = 0;
  spectrum.s_start = 0;
  spectrum.s_inc = 1.;
  spectrum.status = 0;
  spectrum.d_spect_array.d_spect_array_len = value.size();
  spectrum.d_spect_array.d_spect_array_val = (float*)value.data(); // casts const-ness away, but will not be modified
                                                                   // (hopefully)
  ed.set(&spectrum);
  // obtain location pointer
  ad.adr(name.c_str());
  EqFct* p = eq_get(&ad);
  ASSERT(p != NULL, std::string("Could not get location for property ") + name);
  // set spectrum
  p->lock();
  p->set(&ad, &ed, &res);
  p->unlock();
  // check for error
  ASSERT(res.error() == 0, std::string("Error writing spectrum property ") + name);
}

void DoocsServerTestHelper::doocsSetIIII(const std::string& name, const std::vector<int>& value) {
  EqAdr ad;
  EqData ed, res;
  ASSERT(value.size() == 4, std::string("Invalid input size, must be 4"));

  // fill IIII data structure
  IIII iiii;
  iiii.i1_data = value[0];
  iiii.i2_data = value[1];
  iiii.i3_data = value[2];
  iiii.i4_data = value[3];
  ed.set(&iiii);

  // obtain location pointer
  ad.adr(name.c_str());
  EqFct* p = eq_get(&ad);
  ASSERT(p != NULL, std::string("Could not get location for property ") + name);
  // set spectrum
  p->lock();
  p->set(&ad, &ed, &res);
  p->unlock();
  // check for error
  ASSERT(res.error() == 0, std::string("Error writing IIII property ") + name);
}

/**********************************************************************************************************************/

template<>
std::string DoocsServerTestHelper::doocsGet<std::string>(const std::string& name) {
  EqAdr ad;
  EqData ed, res;
  // obtain location pointer
  ad.adr(name);
  EqFct* p = eq_get(&ad);
  ASSERT(p != NULL, std::string("Could not get location for property ") + name);
  // obtain value
  p->lock();
  p->get(&ad, &ed, &res);
  p->unlock();
  // check for errors
  ASSERT(res.error() == 0, std::string("Error reading property ") + name);
  // return requested type
  return res.get_string();
}
