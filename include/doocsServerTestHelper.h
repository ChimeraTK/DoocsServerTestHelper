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
#include <atomic>
#include <type_traits>
#include <signal.h>

#include <eq_fct.h>

/** Handy assertion macro */
#define ASSERT( condition, error_message )                                                              \
    if(! (condition) ) {                                                                                \
      std::cerr << "Assertion failed (" << #condition << "): " << error_message << std::endl;            \
      exit(1);                                                                                          \
    }

/** Collection of helper routines to test DOOCS servers: control the DOOCS update() and
 *  interrupt_usr1() functions and access properties directly through pointers (instead of the RPC
 *  interface). The class has only static functions, so there is no need to create an instance.
 *
 *  Important: You need to set the update rate in the DOOCS server configuration to the following exact
 *  values, or your test will not be able to control update():
 *  "SVR.RATE: 57005 48879 0 0" */

class DoocsServerTestHelper {

  public:

    /** If doNotProcessSignalsInDoocs is set to true, DOOCS will not receive any signals
     *  via sigwait() to process. This allows catching signals via signal handlers (e.g. sigaction).
     */
    static void setDoNotProcessSignalsInDoocs(bool doNotProcessSignalsInDoocs = true);

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

    /** magic sleep time to identify the nanosleep to intercept */
    const static int magic_sleep_time_sec;
    const static int magic_sleep_time_usec;

  protected:

    friend int ::nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining);
    friend int ::sigwait(__const sigset_t *__restrict __set, int *__restrict __sig);

    /** mutex and flag to trigger update() */
    static std::mutex update_mutex;
    static std::atomic<bool> allowUpdate;

    /** mutex and flag to trigger interrupt_usr1() */
    static std::mutex sigusr1_mutex;
    static std::atomic<bool> allowSigusr1;
    
    /** unique locks for */
    static std::unique_lock<std::mutex> update_lock;
    static std::unique_lock<std::mutex> sigusr1_lock;

    /** do not process any signals in DOOCS (to allow installing signal handlers instead) */
    static std::atomic<bool> doNotProcessSignalsInDoocs;

    /** internal event counter */
    static int event;

};

/**********************************************************************************************************************/

template<typename TYPE>
void DoocsServerTestHelper::doocsSet( const std::string &name, TYPE value ) {
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
void DoocsServerTestHelper::doocsSet( const std::string &name, const std::vector<TYPE> &value ) {
    EqAdr ad;
    EqData ed, res;

    // set type and length
    if(typeid(TYPE) == typeid(int)) {
      ed.set_type(DATA_A_INT);
    }
    else if(typeid(TYPE) == typeid(short)) {
      ed.set_type(DATA_A_SHORT);
    }
    else if(typeid(TYPE) == typeid(long long int) || typeid(TYPE) == typeid(int32_t)) {
      ed.set_type(DATA_A_LONG);
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

template<typename TYPE>
TYPE DoocsServerTestHelper::doocsGet( const char *name ) {
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
std::string DoocsServerTestHelper::doocsGet<std::string>( const char *name );

/**********************************************************************************************************************/

template<typename TYPE>
std::vector<TYPE> DoocsServerTestHelper::doocsGetArray( const char *name ) {
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
      if(res.type() != DATA_A_LONG) {
        for(int i=0; i<res.length(); i++) val.push_back(res.get_int(i));
      }
      else {
        for(int i=0; i<res.length(); i++) val.push_back(res.get_long(i));
      }
    }
    else if(std::is_floating_point<TYPE>()) {
      for(int i=0; i<res.length(); i++) val.push_back(res.get_float(i));
    }
    else {
      ASSERT(false, "Wrong type passed as tempalate argument.");
    }
    return val;
}

#endif // DOOCS_SERVER_TEST_HELPER_H
