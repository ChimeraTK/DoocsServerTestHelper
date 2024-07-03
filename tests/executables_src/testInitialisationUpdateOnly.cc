#define BOOST_TEST_MODULE testApplication

#include "testDoocsServerTestHelper_skeleton.h"

using namespace boost::unit_test_framework;

#define CHECK_TIMEOUT(condition, maxMilliseconds)                                                                      \
  {                                                                                                                    \
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();                                       \
    while(!(condition)) {                                                                                              \
      bool timeout_reached = (std::chrono::steady_clock::now() - t0) > std::chrono::milliseconds(maxMilliseconds);     \
      BOOST_CHECK(!timeout_reached);                                                                                   \
      if(timeout_reached) break;                                                                                       \
      usleep(1000);                                                                                                    \
    }                                                                                                                  \
  }

void HelperTest::testRoutineBody() {
  std::atomic<bool> flag{};

  // check if nanosleep with different time than the magic number works
  struct timespec nonMagicSleepTime {
    0, 10
  };
  nanosleep(&nonMagicSleepTime, nullptr);

  // allow the update thread to enter nanosleep
  allowUpdate();

  // test a full update cycle
  flag = false;
  std::thread t1([&flag] {
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowUpdate();
  CHECK_TIMEOUT(flag == true, 5000);
  t1.join();
  waitUpdate();

  // test a second full update cycle
  flag = false;
  std::thread t2([&flag] {
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowUpdate();
  CHECK_TIMEOUT(flag == true, 5000);
  t2.join();
  waitUpdate();

  // check if nanosleep with different time than the magic number works
  struct timespec nonMagicSleepTime2 {
    1, 0
  };
  nanosleep(&nonMagicSleepTime2, nullptr);
}

BOOST_AUTO_TEST_CASE(testUpdateOnly) {
  HelperTest test;
  test.testRoutine();
}