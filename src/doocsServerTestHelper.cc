/*
 * doocsServerTestHelper.cc
 *
 *  Created on: May 5, 2016
 *      Author: Martin Hierholzer
 */

#include "doocsServerTestHelper.h"

#include <doocs/EqFctSvr.h>
#include <sys/types.h>

#include <eq_fct.h>
#include <unistd.h>

#include <csignal>
#include <ctime>

/**********************************************************************************************************************/

// pointer to server location
extern EqFctSvr* server_eq;

/**********************************************************************************************************************/

DoocsServerTestHelper::Data DoocsServerTestHelper::data{};

/**********************************************************************************************************************/

extern "C" int sigwait(__const sigset_t* __restrict set, int* __restrict sig) {
  // Using a static thread_local std::unique_lock<std::mutex> directly inside an extern "C" function seems to cause
  // weird behaviour (unlocking during destruction happens in wrong thread or twice, seen by TSAN), hence we defer to a
  // true C++ function here.
  return DoocsServerTestHelper::sigwait(set, sig);
}

/**********************************************************************************************************************/

int DoocsServerTestHelper::sigwait(__const sigset_t* __restrict set, int* __restrict sig) {
  // call original-equivalent version if SIGUSR1 is not in the set
  if(!sigismember(set, SIGUSR1)) {
    // first wait on the specified signals
    siginfo_t siginfo;
    int iret = sigwaitinfo(set, &siginfo);
    *sig = siginfo.si_signo;
    // if (in the mean time) doNotProcessSignalsInDoocs has been set by the test
    // code, wait on an empty set (-> forever)
    if(DoocsServerTestHelper::data.doNotProcessSignalsInDoocs) {
      sigset_t emptySet;
      sigemptyset(&emptySet);
      iret = sigwaitinfo(&emptySet, &siginfo);
      *sig = siginfo.si_signo;
    }
    return iret;
  }

  // SIGUSR1 is in the set: wait until sigusr1 reqested via mutex
  static thread_local std::unique_lock<std::mutex> sigwait_sigusr1_lock{DoocsServerTestHelper::data.sigusr1_mutex};

  if(DoocsServerTestHelper::data.do_shutdown) {
    *sig = SIGUSR1;
    assert(sigwait_sigusr1_lock.owns_lock());
    return 0;
  }

  do {
    if(sigwait_sigusr1_lock.owns_lock()) {
      sigwait_sigusr1_lock.unlock();
    }
    usleep(1);
    sigwait_sigusr1_lock.lock();
  } while(!DoocsServerTestHelper::data.allowSigusr1 && !DoocsServerTestHelper::data.do_shutdown);
  DoocsServerTestHelper::data.allowSigusr1 = false;

  // return a SIGUSR1
  *sig = SIGUSR1;
  assert(sigwait_sigusr1_lock.owns_lock());
  return 0;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::initialise(doocs::Server* server) {
  server->set_update_delay_fct(&DoocsServerTestHelper::waitForUpdate);
  data.is_initialised = true;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::initialise(HelperTest*) {
  // I just put the HelperTest into the signaure to indicate that this function is not part of the regular API and
  // only used in tests of the DoocsServerTestHelper itself.
  data.is_initialised = true;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::waitForUpdate(const doocs::Server* /*server*/) {
  if(data.do_shutdown) {
    return;
  }
  static thread_local std::unique_lock<std::mutex> waitForUpdate_update_lock{DoocsServerTestHelper::data.update_mutex};
  do {
    if(waitForUpdate_update_lock.owns_lock()) {
      waitForUpdate_update_lock.unlock();
    }
    usleep(1);
    waitForUpdate_update_lock.lock();
  } while(!data.allowUpdate && !data.do_shutdown);
  data.allowUpdate = false;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::setDoNotProcessSignalsInDoocs(bool _doNotProcessSignalsInDoocs) {
  data.doNotProcessSignalsInDoocs = _doNotProcessSignalsInDoocs;
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runSigusr1() {
  data.allowSigusr1 = true;
  do {
    if(data.sigusr1_lock.owns_lock()) {
      data.sigusr1_lock.unlock();
    }
    usleep(1);
    data.sigusr1_lock.lock();
  } while(data.allowSigusr1);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::runUpdate() {
  if(!data.is_initialised) {
    throw std::logic_error("DoocsServerTestHelper::runUpdate() called  without calling initialise() first.");
  }
  data.allowUpdate = true;
  do {
    if(data.update_lock.owns_lock()) {
      data.update_lock.unlock();
    }
    usleep(1);
    data.update_lock.lock();
  } while(data.allowUpdate);
}

/**********************************************************************************************************************/

void DoocsServerTestHelper::shutdown() {
  int myBuildPhase;

  extern int build_phase;
  extern std::mutex mx_svr;
  {
    std::unique_lock<std::mutex> lk_svr{mx_svr};
    myBuildPhase = build_phase;
  }

  // The global variable "myBuildPhase" from DOOCS determines the state the DOOCS server is in. If build_phase is < 2,
  // eq_exit() will immediately terminate the process by calling exit(), which is not wanted here.
  if(myBuildPhase > 1) {
    eq_exit();
  }
  usleep(100000);
  data.do_shutdown = true;
  data.allowUpdate = true;
  data.allowSigusr1 = true;
  if(data.update_lock.owns_lock()) {
    data.update_lock.unlock();
  }
  if(data.sigusr1_lock.owns_lock()) {
    data.sigusr1_lock.unlock();
  }
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
  ad.adr(name);
  EqFct* p = eq_get(&ad);
  ASSERT(p != nullptr, std::string("Could not get location for property ") + name);
  // set spectrum
  size_t retry = 1000;
  do {
    p->lock();
    p->set(&ad, &ed, &res);
    p->unlock();
    if(res.error() == 0) {
      break;
    }
    usleep(10000);
  } while(--retry > 0);
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
  ad.adr(name);
  EqFct* p = eq_get(&ad);
  ASSERT(p != nullptr, std::string("Could not get location for property ") + name);
  // set spectrum
  size_t retry = 1000;
  do {
    p->lock();
    p->set(&ad, &ed, &res);
    p->unlock();
    if(res.error() == 0) {
      break;
    }
    usleep(10000);
  } while(--retry > 0);
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
  ASSERT(p != nullptr, std::string("Could not get location for property ") + name);
  // obtain value
  size_t retry = 1000;
  do {
    p->lock();
    p->get(&ad, &ed, &res);
    p->unlock();
    if(res.error() == 0) {
      break;
    }
    usleep(10000);
  } while(--retry > 0);
  // check for errors
  ASSERT(res.error() == 0, std::string("Error reading property ") + name);
  // return requested type
  return res.get_string();
}
