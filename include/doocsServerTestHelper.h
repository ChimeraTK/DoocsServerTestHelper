/*
 * doocsServerTestHelper.h - collection of helper routines to test DOOCS
 * servers: control the DOOCS update() and interrupt_usr1() functions and access
 * properties directly through pointers (instead of the RPC interface).
 *
 *  Created on: May 2, 2016
 *      Author: Martin Hierholzer
 */

#ifndef DOOCS_SERVER_TEST_HELPER_H
#define DOOCS_SERVER_TEST_HELPER_H

#include <doocs/Server.h>
#include <eq_fct.h>
#include <type_traits>

#include <atomic>
#include <cassert>
#include <csignal>
#include <future>
#include <iostream>
#include <mutex>

class HelperTest;

/** Handy assertion macro */
#define ASSERT(condition, error_message)                                                                               \
  if(!(condition)) {                                                                                                   \
    std::cerr << "Assertion failed (" << #condition << "): " << (error_message) << std::endl;                          \
    assert(false);                                                                                                     \
  }

/** Collection of helper routines to test DOOCS servers: control the DOOCS
 * update() and interrupt_usr1() functions and access properties directly
 * through pointers (instead of the RPC interface). The class has only static
 * functions, so there is no need to create an instance.
 *
 * Warning: When linking against the DoocsServerTestHelper library, doocs::Server::wait_for_update()
 * and sigwait() will be replaced and calls to those functions will be
 * intercepted to wait until the corresponding function (runSigusr1() or
 * runUpdate()) has been called by the test code, if the passed arguments match
 * the expected signature. If this library is linked against an ordinary DOOCS
 * server without a test routine calling those functions, the server will hang
 * in a dead lock situation. Use this library only in combination with an
 * appropriate test routine!
 */

class DoocsServerTestHelper {
 public:
  /** If doNotProcessSignalsInDoocs is set to true, DOOCS will not receive any
   * signals via sigwait() to process. This allows catching signals via signal
   * handlers (e.g. sigaction).
   */
  static void setDoNotProcessSignalsInDoocs(bool doNotProcessSignalsInDoocs = true);

  /** Needed to register the wait_for_upate() function with the doocs Server object.
   * This can only happen after the server object has been created, but has to happen
   * before the server is started.
   */
  static void initialise(doocs::Server*);

  /** Initialisation function for use in the test for the test helper.
   */
  static void initialise(HelperTest*);

  /** trigger doocs to run interrupt_usr1() in all locations and wait until the
   * processing is finished */
  static void runSigusr1();

  /** trigger doocs to run update() in all locations and wait until the
   * processing is finished */
  static void runUpdate();

  /** shutdown the doocs server */
  static void shutdown();

  /** set a DOOCS property
   *  "name" is the property name in the form "//<location>/<property>"
   *  "value" is the value to be set
   */
  template<typename TYPE>
  static void doocsSet(const std::string& name, TYPE value);

  /** set a DOOCS property - array version
   *  "name" is the property name in the form "//<location>/<property>"
   *  "value" is the value to be set
   */
  template<typename TYPE>
  static void doocsSet(const std::string& name, const std::vector<TYPE>& value);

  /** set a DOOCS property - spectrum version
   *  "name" is the property name in the form "//<location>/<property>"
   *  "value" is the value to be set
   */
  static void doocsSetSpectrum(const std::string& name, const std::vector<float>& value);

  /** set a DOOCS property - IIII version
   *  "name" is the property name in the form "//<location>/<property>"
   *  "value" is the value to be set
   */
  static void doocsSetIIII(const std::string& name, const std::vector<int>& value);

  /** get a scalar DOOCS property
   *  "name" is the property name in the form "//<location>/<property>"
   */
  template<typename TYPE>
  static TYPE doocsGet(const std::string& name);

  /** get an array DOOCS property
   *  "name" is the property name in the form "//<location>/<property>"
   */
  template<typename TYPE>
  static std::vector<TYPE> doocsGetArray(const std::string& name);

  /** Function to replace the wait_for_update() function in the server. It blocks
   *  until runUpdate() has been called in the test. Public to be able to unit-test it.
   */
  static void waitForUpdate(const doocs::Server* server);

 protected:
  friend int ::sigwait(__const sigset_t* __restrict _set, int* __restrict _sig);

  /** mutex and flag to trigger update() */
  static std::mutex update_mutex;
  static std::atomic<bool> allowUpdate;

  /** mutex and flag to trigger interrupt_usr1() */
  static std::mutex sigusr1_mutex;
  static std::atomic<bool> allowSigusr1;

  /** unique locks for */
  static std::unique_lock<std::mutex> update_lock;
  static std::unique_lock<std::mutex> sigusr1_lock;

  /** do not process any signals in DOOCS (to allow installing signal handlers
   * instead) */
  static std::atomic<bool> doNotProcessSignalsInDoocs;

  /** internal event counter */
  static int event;

  static std::atomic<bool> is_initialised; // flag to check whether the server test hook has been registed
  static std::atomic<bool> do_shutdown;    // flag to cleanly exit wait_for_update
};

/**********************************************************************************************************************/

template<typename TYPE>
void DoocsServerTestHelper::doocsSet(const std::string& name, TYPE value) {
  EqAdr ad;
  EqData ed, res;
  // obtain location pointer
  ad.adr(name);
  EqFct* p = eq_get(&ad);
  ASSERT(p != nullptr, std::string("Could not get location for property ") + name);
  // set value
  ed.set(value);
  size_t retry = 1000;
  do {
    p->lock();
    p->set(&ad, &ed, &res);
    p->unlock();
  } while(res.error() != 0 && --retry > 0);
  // check for error
  ASSERT(res.error() == 0, std::string("Error writing property ") + name + ": " + res.get_string());
}

/**********************************************************************************************************************/

template<typename TYPE>
void DoocsServerTestHelper::doocsSet(const std::string& name, const std::vector<TYPE>& value) {
  EqAdr ad;
  EqData ed, res;

  // set type and length
  if(typeid(TYPE) == typeid(int)) {
    ed.set_type(DATA_A_INT);
  }
  else if(typeid(TYPE) == typeid(short)) {
    ed.set_type(DATA_A_SHORT);
  }
  else if(typeid(TYPE) == typeid(long long int) || typeid(TYPE) == typeid(int32_t) || typeid(TYPE) == typeid(int64_t)) {
    ed.set_type(DATA_A_LONG);
  }
  else if(typeid(TYPE) == typeid(float)) {
    ed.set_type(DATA_A_FLOAT);
  }
  else if(typeid(TYPE) == typeid(double)) {
    ed.set_type(DATA_A_DOUBLE);
  }
  else {
    ASSERT(false, std::string("Unknown data type: ") + std::string(typeid(TYPE).name()));
  }
  ed.length(value.size());

  // fill spectrum data structure
  for(int i = 0; i < value.size(); i++) {
    ed.set(value[i], i);
  }
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
  } while(res.error() != 0 && --retry > 0);
  // check for error
  ASSERT(res.error() == 0, std::string("Error writing array property ") + name + ": " + res.get_string());
}

/**********************************************************************************************************************/

template<typename TYPE>
TYPE DoocsServerTestHelper::doocsGet(const std::string& name) {
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
  } while(res.error() != 0 && --retry > 0);
  // return requested type (note: std::string is handled in a template
  // specialisation)
  if(std::is_integral<TYPE>()) {
    return res.get_int();
  }
  if(std::is_floating_point<TYPE>()) {
    return res.get_float();
  }
  ASSERT(false, "Wrong type passed as tempalate argument.");
}

/**********************************************************************************************************************/

template<>
std::string DoocsServerTestHelper::doocsGet<std::string>(const std::string& name);

/**********************************************************************************************************************/

template<typename TYPE>
std::vector<TYPE> DoocsServerTestHelper::doocsGetArray(const std::string& name) {
  EqAdr ad;
  EqData ed, res;
  // obtain location pointer
  ad.adr(name);
  EqFct* p = eq_get(&ad);
  ASSERT(p != nullptr, std::string("Could not get location for property ") + name);
  // for D_Spectrum: set IIII structure to obtain always the latest buffer
  IIII iiii;
  iiii.i1_data = -1;
  iiii.i2_data = -1;
  iiii.i3_data = -1;
  iiii.i4_data = -1;
  ed.set(&iiii);
  // obtain values
  size_t retry = 1000;
  do {
    p->lock();

    // Try to get the data with parameters for a spectrum
    p->get(&ad, &ed, &res);
    if(res.error() == eq_errors::not_implemeted) {
      // if that fails, assume we have a plain array, and just not pass the second parameter at all
      p->get(&ad, nullptr, &res);
    }
    p->unlock();
  } while(res.error() != 0 && --retry > 0);
  // check for errors
  ASSERT(res.error() == 0, std::string("Error reading property ") + name);

  // copy to vector and return it
  std::vector<TYPE> val;
  if(std::is_integral<TYPE>()) {
    if(res.type() != DATA_A_LONG) {
      for(int i = 0; i < res.length(); i++) {
        val.push_back(res.get_int(i));
      }
    }
    else {
      for(int i = 0; i < res.length(); i++) {
        val.push_back(res.get_long(i));
      }
    }
  }
  else if(std::is_floating_point<TYPE>()) {
    for(int i = 0; i < res.length(); i++) {
      val.push_back(res.get_float(i));
    }
  }
  else {
    ASSERT(false, "Wrong type passed as tempalate argument.");
  }
  return val;
}

#endif // DOOCS_SERVER_TEST_HELPER_H
