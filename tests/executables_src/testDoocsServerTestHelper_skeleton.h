/**
 *  testDoocsServerTestHelper_skeleton.h
 *
 *  Skeleton header file for a sequence of tests which have to be in separate
 * executables but share most of its code.
 */

#include <boost/test/included/unit_test.hpp>
#include <future>
#include <mutex>
#include <queue>

#include <signal.h>
#include <time.h>

#include "doocsServerTestHelper.h"

using namespace boost::unit_test_framework;

// flag whether we are done with testing. Must be global, since we need to
// access it from the SIGINT handler
std::atomic<bool> flagTerminate;

class HelperTest {
 public:
  void testRoutine();
  void testRoutineBody(); // to be implemented for each test

  // routines running in separate threads each with a loop simulating the
  // sigusr1/update triggering behaviour of DOOCS
  void sigusr1();
  void update();

  // helper routines to allow the threads running sigwait resp. nanosleep one
  // (more) time
  void allowSigusr1();
  void allowUpdate();

  // helper routines to wait until the threads have completed their sigwait
  // resp. nanosleep
  void waitSigusr1();
  void waitUpdate();

  // queues of futures to communicate between threads:
  std::queue<std::future<void>> qSigusr1Start;
  std::queue<std::future<void>> qSigusr1Done;
  std::queue<std::future<void>> qUpdateStart;
  std::queue<std::future<void>> qUpdateDone;
  std::mutex queueLock;

  // only used from the test routines thread: the last promises sent to the
  // queue (as futures)
  std::promise<void> lastPromiseSigusr1;
  std::promise<void> lastPromiseUpdate;

  HelperTest();
};

class HelperTestSuite : public test_suite {
 public:
  HelperTestSuite() : test_suite("DoocsServerTestHelper class test suite") {
    boost::shared_ptr<HelperTest> HelperTestPtr(new HelperTest());

    add(BOOST_CLASS_TEST_CASE(&HelperTest::testRoutine, HelperTestPtr));
  }
};
test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/ []) {
  framework::master_test_suite().p_name.value = "DoocsServerTestHelper class test suite";
  framework::master_test_suite().add(new HelperTestSuite());

  return NULL;
}

HelperTest::HelperTest() {
  // this allows to call DoocsServerTestHelper::runUpdate()
  DoocsServerTestHelper::initialise(this);
}

void HelperTest::sigusr1() {
  int sig;
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  std::cout << "threadSigusr1 started." << std::endl;

  // initialise the future queue
  std::promise<void> lastPromise;
  {
    std::lock_guard<std::mutex> lock{queueLock};
    qSigusr1Done.push(lastPromise.get_future());
  }

  // repeating loop until the termination flag is set
  while(!flagTerminate) {
    // wait until the test routine tells us to proceed
    std::cout << "threadSigusr1 waiting..." << std::endl;
    std::future<void> future;
    {
      std::lock_guard<std::mutex> lock{queueLock};
      future = std::move(qSigusr1Start.front());
    }
    std::cout << "threadSigusr1 waiting (2)..." << std::endl;
    future.get();
    std::cout << "threadSigusr1 waiting (3)..." << std::endl;
    {
      std::lock_guard<std::mutex> lock{queueLock};
      qSigusr1Start.pop();
    }
    if(flagTerminate) break;

    // execute sigwait
    std::cout << "sigwait ->" << std::endl;
    sigwait(&set, &sig);
    std::cout << "<- sigwait" << std::endl;

    // tell the test routine that sigwait was terminated
    std::promise<void> newPromise = std::promise<void>();
    {
      std::lock_guard<std::mutex> lock{queueLock};
      qSigusr1Done.push(newPromise.get_future());
    }
    lastPromise.set_value();
    lastPromise.swap(newPromise);
  }
  std::cout << "threadSigusr1 terminated." << std::endl;
}

void HelperTest::update() {
  std::cout << "threadUpdate started." << std::endl;

  // initialise the future queue
  std::promise<void> lastPromise;
  {
    std::lock_guard<std::mutex> lock{queueLock};
    qUpdateDone.push(lastPromise.get_future());
  }

  // repeating loop until the termination flag is set
  while(!flagTerminate) {
    // wait until the test routine tells us to proceed
    std::cout << "threadUpdate waiting..." << std::endl;
    std::future<void> future;
    {
      std::lock_guard<std::mutex> lock{queueLock};
      future = std::move(qUpdateStart.front());
    }
    std::cout << "threadUpdate waiting (2)..." << std::endl;
    future.get();
    std::cout << "threadUpdate waiting (3)..." << std::endl;
    {
      std::lock_guard<std::mutex> lock{queueLock};
      qUpdateStart.pop();
    }
    if(flagTerminate) break;

    DoocsServerTestHelper::wait_for_update(nullptr);

    // tell the test routine that nanosleep was terminated
    std::promise<void> newPromise = std::promise<void>();
    {
      std::lock_guard<std::mutex> lock{queueLock};
      qUpdateDone.push(newPromise.get_future());
    }
    lastPromise.set_value();
    lastPromise.swap(newPromise);
  }
}

void HelperTest::allowSigusr1() {
  std::cout << "Allow sigusr1..." << std::endl;
  std::lock_guard<std::mutex> lock{queueLock};
  std::promise<void> newPromise = std::promise<void>();
  qSigusr1Start.push(newPromise.get_future());
  lastPromiseSigusr1.set_value();
  lastPromiseSigusr1.swap(newPromise);
  std::cout << "sigusr1 allowed..." << std::endl;
}

void HelperTest::allowUpdate() {
  std::cout << "Allow update..." << std::endl;
  std::lock_guard<std::mutex> lock{queueLock};
  std::promise<void> newPromise = std::promise<void>();
  qUpdateStart.push(newPromise.get_future());
  lastPromiseUpdate.set_value();
  lastPromiseUpdate.swap(newPromise);
  std::cout << "update allowed..." << std::endl;
}

void HelperTest::waitSigusr1() {
  std::cout << "Wait for sigusr1 done..." << std::endl;
  std::future<void> future;
  {
    std::lock_guard<std::mutex> lock{queueLock};
    future = std::move(qSigusr1Done.front());
  }
  future.get();
  {
    std::lock_guard<std::mutex> lock{queueLock};
    qSigusr1Done.pop();
  }
  std::cout << "sigusr1 is done." << std::endl;
}

void HelperTest::waitUpdate() {
  std::cout << "Wait for update done..." << std::endl;
  std::future<void> future;
  {
    std::lock_guard<std::mutex> lock{queueLock};
    future = std::move(qUpdateDone.front());
  }
  future.get();
  {
    std::lock_guard<std::mutex> lock{queueLock};
    qUpdateDone.pop();
  }
  std::cout << "update is done." << std::endl;
}

void sigintHandler(int) {
  if(flagTerminate) return;
  std::cout << "Ctrl+C received." << std::endl;
  exit(1);
}

void HelperTest::testRoutine() {
  // block sigusr1 for this process and thread (otherwise the signal would
  // terminate the process)
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);
  pthread_sigmask(SIG_BLOCK, &set, NULL);

  signal(SIGINT, sigintHandler);

  flagTerminate = false;

  // initialise the future queues (only those which are being filled from this
  // thread)
  qSigusr1Start.push(lastPromiseSigusr1.get_future());
  qUpdateStart.push(lastPromiseUpdate.get_future());

  // start the test threads
  std::cout << "Starting threadSigusr1..." << std::endl;
  std::thread threadSigusr1(std::bind(&HelperTest::sigusr1, this));
  std::cout << "Starting threadUpdate..." << std::endl;
  std::thread threadUpdate(std::bind(&HelperTest::update, this));

  // execute the actual test
  testRoutineBody();

  // shut down the test procedure
  flagTerminate = true;
  allowUpdate();
  allowSigusr1();
  std::cout << "DoocsServerTestHelper::shutdown() ->" << std::endl;
  DoocsServerTestHelper::shutdown();
  std::cout << "<- DoocsServerTestHelper::shutdown()" << std::endl;

  std::cout << "Terminating..." << std::endl;
  lastPromiseUpdate.set_value();
  lastPromiseSigusr1.set_value();
  std::cout << "threadSigusr1 joining..." << std::endl;
  threadSigusr1.join();
  std::cout << "threadUpdate joining..." << std::endl;
  threadUpdate.join();
  std::cout << "Finished." << std::endl;
}

// necessary exported symbols to satisfy the DOOCS server lib
const char* object_name = "";
void interrupt_usr1_prolog(int) {}
void post_init_epilog() {}
void refresh_epilog() {}
void refresh_prolog() {}
void eq_init_epilog() {}
void eq_init_prolog() {}
void eq_cancel() {}
void post_init_prolog() {}
void eq_create(int, void*) {}
void interrupt_usr1_epilog(int) {}
