/*
 * doocsServerTestHelper.cc
 *
 *  Created on: May 5, 2016
 *      Author: Martin Hierholzer
 */

#include "doocsServerTestHelper.h"

#include <doocs/EqFctSvr.h>
#include <eq_fct.h>
#include <sys/types.h>

#include <signal.h>
#include <time.h>
#include <unistd.h>

/**********************************************************************************************************************/

// pointer to server location
extern EqFctSvr* server_eq;

/**********************************************************************************************************************/

extern "C" int sigwait(__const sigset_t* __restrict set, int* __restrict sig) {
  // call original-equivalent version if SIGUSR1 is not in the set
  if(!sigismember(set, SIGUSR1)) {
    // first wait on the specified signals
    siginfo_t siginfo;
    int iret = sigwaitinfo(set, &siginfo);
    *sig = siginfo.si_signo;
    // if (in the mean time) doNotProcessSignalsInDoocs has been set by the test
    // code, wait on an empty set (-> forever)
    if(DoocsServerTestHelper::doNotProcessSignalsInDoocs) {
      sigset_t emptySet;
      sigemptyset(&emptySet);
      iret = sigwaitinfo(&emptySet, &siginfo);
      *sig = siginfo.si_signo;
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
  *sig = SIGUSR1;
  return 0;
}

/**********************************************************************************************************************/

std::atomic<bool> DoocsServerTestHelper::allowUpdate(false);
std::atomic<bool> DoocsServerTestHelper::allowSigusr1(false);
std::atomic<bool> DoocsServerTestHelper::doNotProcessSignalsInDoocs(false);
std::mutex DoocsServerTestHelper::update_mutex;
std::mutex DoocsServerTestHelper::sigusr1_mutex;
std::unique_lock<std::mutex> DoocsServerTestHelper::update_lock(DoocsServerTestHelper::update_mutex);
std::unique_lock<std::mutex> DoocsServerTestHelper::sigusr1_lock(DoocsServerTestHelper::sigusr1_mutex);
std::atomic<bool> DoocsServerTestHelper::is_initialised(false);
std::atomic<bool> DoocsServerTestHelper::do_shutdown(false);

/**********************************************************************************************************************/

void DoocsServerTestHelper::initialise(doocs::Server* server) {
  server->set_update_delay_fct(&DoocsServerTestHelper::wait_for_update);
  is_initialised = true;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::initialise(HelperTest*) {
  // I just put the HelperTest into the signaure to indicate that this function is not part of the regular API and only
  // used in tests of the DoocsServerTestHelper itself.
  is_initialised = true;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::wait_for_update(const doocs::Server* /*server*/) {
  if(do_shutdown) return;
  static thread_local std::unique_lock<std::mutex> update_lock{DoocsServerTestHelper::update_mutex};
  do {
    update_lock.unlock();
    usleep(1);
    update_lock.lock();
  } while(!allowUpdate && !do_shutdown);
  allowUpdate = false;
}

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
  if(!is_initialised) {
    throw std::logic_error("DoocsServerTestHelper::runUpdate() called  without calling initialise() first.");
  }
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
  do_shutdown = true;
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
