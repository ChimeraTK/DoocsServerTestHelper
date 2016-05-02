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
#include <signal.h>

#include <eq_fct.h>

/** Handy assertion macro */
#define ASSERT( condition, error_message )                                                              \
    if(! (condition) ) {                                                                                \
      std::cerr << "Assertion failed (" << #condition << "):" << error_message << std::endl;            \
      exit(1);                                                                                          \
    }

/** Collection of helper routines to test DOOCS servers: control the DOOCS update() and
 *  interrupt_usr1() functions and access properties directly through pointers (instead of the RPC
 *  interface). The class has only static functions, so there is no need to create an instance. */
class doocsServerTestHelper {

  public:

    /// initialise timing control and enable intercepting the required system calls. This will trigger a first
    /// sigusr1 to make sure the server has properly started. Until the server started and processed the first
    /// siguser1, this function will block.
    /// If the optional argument is set to true, DOOCS will not receive any signals via sigwait() to process.
    /// This allows catching signals via signal handlers (e.g. sigaction).
    static void initialise(bool _doNotProcessSignalsInDoocs = false);

    /// trigger doocs to run interrupt_usr1() in all locations and wait until the processing is finished
    static void runSigusr1();

    /// trigger doocs to run update() in all locations and wait until the processing is finished
    static void runUpdate();

    /// shutdown the doocs server
    static void shutdown();

    /**********************************************************************************************************************/
    /** set a DOOCS property
     *  "name" is the property name in the form "//<location>/<property>"
     *  "value" is the value to be set
     */
    template<typename TYPE>
    static void doocsSet( const std::string &name, TYPE value ) {
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
    /** set a DOOCS property - array/spectrum version
     *  "name" is the property name in the form "//<location>/<property>"
     *  "value" is the value to be set
     */
    template<typename TYPE>
    static void doocsSet( const std::string &name, const std::vector<TYPE> &value ) {
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
        ASSERT(res.error() == 0, std::string("Error writing property ")+name);
    }

    /**********************************************************************************************************************/
    /** get a DOOCS property with int type
     *  "name" is the property name in the form "//<location>/<property>"
     */
    static int doocsGet_int( const char *name ) {
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
        // return after check
        ASSERT(res.error() == 0, std::string("Error reading property ")+name);
        return res.get_int();
    }

    /**********************************************************************************************************************/
    /** get a DOOCS property with float type
     *  "name" is the property name in the form "//<location>/<property>"
     */
    static float doocsGet_float( const char *name ) {
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
        //return after check
        ASSERT(res.error() == 0, std::string("Error reading property ")+name);
        return res.get_float();
    }

    /**********************************************************************************************************************/
    /** get a DOOCS property with string type
     *  "name" is the property name in the form "//<location>/<property>"
     */
    static std::string doocsGet_string( const char *name ) {
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
        //return after check
        ASSERT(res.error() == 0, std::string("Error reading property ")+name);
        return res.get_string();
    }

    /**********************************************************************************************************************/
    /** get a DOOCS property with int array type
     *  "name" is the property name in the form "//<location>/<property>"
     */
    static std::vector<int> doocsGet_intArray( const char *name ) {
        EqAdr ad;
        EqData ed, res;
        // obtain location pointer
        ad.adr(name);
        EqFct *p = eq_get(&ad);
        ASSERT(p != NULL, std::string("Could not get location for property ")+name);
        // obtain values
        p->lock();
        p->get(&ad,&ed,&res);
        p->unlock();
        // return as std::vector after check
        ASSERT(res.error() == 0, std::string("Error reading property ")+name);
        std::vector<int> val;
        for(int i=0; i<res.length(); i++) val.push_back(res.get_int(i));
        return val;
    }

    /**********************************************************************************************************************/
    /** get a DOOCS property with float array type
     *  "name" is the property name in the form "//<location>/<property>"
     */
    static std::vector<float> doocsGet_floatArray( const char *name ) {
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
        // return as std::vector after check
        ASSERT(res.error() == 0, std::string("Error reading property ")+name);
        std::vector<float> val;
        for(int i=0; i<res.length(); i++) val.push_back(res.get_float(i));
        return val;
    }

  protected:

    friend int nanosleep(__const struct timespec *__requested_time, struct timespec *__remaining);
    friend int sigwait(__const sigset_t *__restrict __set, int *__restrict __sig);

    /// mutex and flag to trigger update()
    static std::mutex update_mutex;
    static bool allowUpdate;

    /// mutex and flag to trigger interrupt_usr1()
    static std::mutex sigusr1_mutex;
    static bool allowSigusr1;

    /// magic sleep time to identify the nanosleep to intercept
    static int magic_sleep_time_sec;
    static int magic_sleep_time_usec;

    /// enable intercepting system calls
    static bool interceptSystemCalls;

    /// do not process any signals in DOOCS (to allow installing signal handlers instead)
    static bool doNotProcessSignalsInDoocs;

    /// internal event counter
    static int event;

};

#endif // DOOCS_SERVER_TEST_HELPER_H
