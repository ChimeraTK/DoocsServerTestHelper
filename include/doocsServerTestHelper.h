/*
 * doocsServerTestHelper.h - collection of helper routines to test DOOCS servers: control the DOOCS
 *                           update() and interrupt_usr1() functions and access properties directly
 *                           through pointers (instead of the RPC interface). 
 *
 *  Created on: May 2, 2016
 *      Author: Martin Hierholzer
 */

#ifndef DOOCS_SERVER_TEST_HELPER_H
#define DOOCS_SERVER_TEST_HELPER_H

#include <mutex>
#include <future>
#include <type_traits>
#include <signal.h>

#include <eq_fct.h>

/** Handy assertion macro */
#define ASSERT( condition, error_message )                                                              \
    if(! (condition) ) {                                                                                \
      std::cerr << "Assertion failed (" << #condition << "): " << error_message << std::endl;            \
      exit(1);                                                                                          \
    }

namespace mtca4u {

  /** Collection of helper routines to test DOOCS servers: control the DOOCS update() and
   *  interrupt_usr1() functions and access properties directly through pointers (instead of the RPC
   *  interface). The class has only static functions, so there is no need to create an instance. */
  class doocsServerTestHelper {

    public:

      /** initialise timing control and enable intercepting the required system calls. This will trigger a first
       *  sigusr1 to make sure the server has properly started. Until the server started and processed the first
       *  siguser1, this function will block.
       *  If the optional argument is set to true, DOOCS will not receive any signals via sigwait() to process.
       *  This allows catching signals via signal handlers (e.g. sigaction).
       *  Note that update() and interrupt_usr1() may be executed a few times before this function returns. */
      static void initialise(bool _doNotProcessSignalsInDoocs = false);

      /** trigger doocs to run interrupt_usr1() in all locations and wait until the processing is finished */
      static void runSigusr1();

      /** trigger doocs to run update() in all locations and wait until the processing is finished */
      static void runUpdate();

      /** shutdown the doocs server */
      static void shutdown();

      /** set a DOOCS property
       *  "name" is the property name in the form "//<location>/<property>"
       *  "value" is the value to be set
       */
      template<typename TYPE>
      static void doocsSet( const std::string &name, TYPE value );

      /** set a DOOCS property - array version
       *  "name" is the property name in the form "//<location>/<property>"
       *  "value" is the value to be set
       */
      template<typename TYPE>
      static void doocsSet( const std::string &name, const std::vector<TYPE> &value );

      /** set a DOOCS property - spectrum version
       *  "name" is the property name in the form "//<location>/<property>"
       *  "value" is the value to be set
       */
      static void doocsSetSpectrum( const std::string &name, const std::vector<float> &value );

      /** get a scalar DOOCS property
       *  "name" is the property name in the form "//<location>/<property>"
       */
      template<typename TYPE>
      static TYPE doocsGet( const char *name );

      /** get an array DOOCS property
       *  "name" is the property name in the form "//<location>/<property>"
       */
      template<typename TYPE>
      static std::vector<TYPE> doocsGetArray( const char *name );

      /** DEPCRECATED, use templated doocsGet instead. */
      static int doocsGet_int( const char *name );

      /** DEPCRECATED, use templated doocsGet instead. */
      static float doocsGet_float( const char *name );

      /** DEPCRECATED, use templated doocsGet instead. */
      static std::string doocsGet_string( const char *name );

      /** DEPCRECATED, use templated doocsGetArray instead. */
      static std::vector<int> doocsGet_intArray( const char *name );

      /** DEPCRECATED, use templated doocsGetArray instead. */
      static std::vector<float> doocsGet_floatArray( const char *name );

      /** DEPCRECATED, use templated doocsGetArray instead. */
      static std::vector<double> doocsGet_doubleArray( const char *name );

    protected:

      friend int ::nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining);
      friend int ::sigwait(__const sigset_t *__restrict __set, int *__restrict __sig);

      /** mutex and flag to trigger update() */
      static std::mutex update_mutex;
      static bool allowUpdate;

      /** mutex and flag to trigger interrupt_usr1() */
      static std::mutex sigusr1_mutex;
      static bool allowSigusr1;

      /** future and promise to synchronise server startup */
      static std::future<void> serverIsStarted;
      static std::promise<void> setServerIsStarted;

      /** flag set if the nanosleep() function should set the promise setServerIsStarted */
      static bool bootingServer;

      /** magic sleep time to identify the nanosleep to intercept */
      static int magic_sleep_time_sec;
      static int magic_sleep_time_usec;

      /** enable intercepting system calls */
      static bool interceptSystemCalls;

      /** do not process any signals in DOOCS (to allow installing signal handlers instead) */
      static bool doNotProcessSignalsInDoocs;

      /** internal event counter */
      static int event;

  };

  /**********************************************************************************************************************/

  template<typename TYPE>
  void doocsServerTestHelper::doocsSet( const std::string &name, TYPE value ) {
      EqAdr ad;
      EqData ed, res;
      // obtain location pointer
      ad.adr(name.c_str());
      EqFct *p = eq_get(&ad);
      ASSERT(p != NULL, std::string("Could not get location for property ")+name);
      // set value
      ed.set(value);
      p->lock();
      p->set(&ad,&ed,&res);
      p->unlock();
      // check for error
      ASSERT(res.error() == 0, std::string("Error writing property ")+name);
  }

  /**********************************************************************************************************************/

  template<typename TYPE>
  void doocsServerTestHelper::doocsSet( const std::string &name, const std::vector<TYPE> &value ) {
      EqAdr ad;
      EqData ed, res;

      // set type and length
      if(typeid(TYPE) == typeid(int)) {
        ed.set_type(DATA_A_INT);
      }
      else if(typeid(TYPE) == typeid(short)) {
        ed.set_type(DATA_A_SHORT);
      }
      else if(typeid(TYPE) == typeid(float)) {
        ed.set_type(DATA_A_FLOAT);
      }
      else if(typeid(TYPE) == typeid(double)) {
        ed.set_type(DATA_A_DOUBLE);
      }
      else {
        ASSERT(false, std::string("Unknown data type: ")+std::string(typeid(TYPE).name()));
      }
      ed.length(value.size());

      // fill spectrum data structure
      for(int i=0; i<value.size(); i++) ed.set(value[i],i);
      // obtain location pointer
      ad.adr(name.c_str());
      EqFct *p = eq_get(&ad);
      ASSERT(p != NULL, std::string("Could not get location for property ")+name);
      // set spectrum
      p->lock();
      p->set(&ad,&ed,&res);
      p->unlock();
      // check for error
      ASSERT(res.error() == 0, std::string("Error writing array property ")+name+": "+res.get_string());
  }

  /**********************************************************************************************************************/

  void doocsServerTestHelper::doocsSetSpectrum( const std::string &name, const std::vector<float> &value ) {
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

  template<typename TYPE>
  TYPE doocsServerTestHelper::doocsGet( const char *name ) {
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
      // return requested type (note: std::string is handled in a template specialisation)
      if(std::is_integral<TYPE>()) {
        return res.get_int();
      }
      else if(std::is_floating_point<TYPE>()) {
        return res.get_float();
      }
      ASSERT(false, "Wrong type passed as tempalate argument.");
  }

  /**********************************************************************************************************************/

  template<>
  std::string doocsServerTestHelper::doocsGet<std::string>( const char *name ) {
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

  /**********************************************************************************************************************/

  template<typename TYPE>
  std::vector<TYPE> doocsServerTestHelper::doocsGetArray( const char *name ) {
      EqAdr ad;
      EqData ed, res;
      // obtain location pointer
      ad.adr(name);
      EqFct *p = eq_get(&ad);
      ASSERT(p != NULL, std::string("Could not get location for property ")+name);
      // for D_BSpectrum: set IIII structure to obtain always the latest buffer (shouldn't hurt for others)
      IIII iiii;
      iiii.i1_data = -1;
      iiii.i2_data = -1;
      iiii.i3_data = -1;
      iiii.i4_data = -1;
      ed.set(&iiii);
      // obtain values
      p->lock();
      p->get(&ad,&ed,&res);
      p->unlock();
      // check for errors
      ASSERT(res.error() == 0, std::string("Error reading property ")+name);
      // copy to vector and return it
      std::vector<TYPE> val;
      if(std::is_integral<TYPE>()) {
        for(int i=0; i<res.length(); i++) val.push_back(res.get_int(i));
      }
      else if(std::is_floating_point<TYPE>()) {
        for(int i=0; i<res.length(); i++) val.push_back(res.get_float(i));
      }
      else {
        ASSERT(false, "Wrong type passed as tempalate argument.");
      }
      return val;
  }

  /**********************************************************************************************************************/

  int doocsServerTestHelper::doocsGet_int( const char *name ) {
      return doocsGet<int>(name);
  }

  /**********************************************************************************************************************/

  float doocsServerTestHelper::doocsGet_float( const char *name ) {
      return doocsGet<float>(name);
  }

  /**********************************************************************************************************************/

  std::string doocsServerTestHelper::doocsGet_string( const char *name ) {
      return doocsGet<std::string>(name);
  }

  /**********************************************************************************************************************/

  std::vector<int> doocsServerTestHelper::doocsGet_intArray( const char *name ) {
      return doocsGetArray<int>(name);
  }

  /**********************************************************************************************************************/

  std::vector<float> doocsServerTestHelper::doocsGet_floatArray( const char *name ) {
      return doocsGetArray<float>(name);
  }

  /**********************************************************************************************************************/

  std::vector<double> doocsServerTestHelper::doocsGet_doubleArray( const char *name ) {
      return doocsGetArray<double>(name);
  }

} /* namespace mtca4u */

#endif // DOOCS_SERVER_TEST_HELPER_H
