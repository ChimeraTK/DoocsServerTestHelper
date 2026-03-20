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
  std::cout << "HelperTest::testRoutineBody() 1" << std::endl;

  // allow the update thread to enter nanosleep and the sigusr1 thread to enter
  // sigwait
  allowUpdate();
  allowSigusr1();

  // test a full update cycle
  flag = false;
  std::thread t1([&] {
    usleep(100000);
    BOOST_CHECK(flag == false);
    allowUpdate();
    CHECK_TIMEOUT(flag == true, 5000);
  });
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  flag = true;
  t1.join();
  waitUpdate();

  // test a full sigusr1 cycle
  flag = false;
  std::thread t2([&] {
    usleep(100000);
    BOOST_CHECK(flag == false);
    allowSigusr1();
    CHECK_TIMEOUT(flag == true, 5000);
  });
  std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
  DoocsServerTestHelper::runSigusr1();
  std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
  flag = true;
  t2.join();
  waitSigusr1();

  // test a second full update cycle
  flag = false;
  std::thread t3([&] {
    usleep(100000);
    BOOST_CHECK(flag == false);
    allowUpdate();
    CHECK_TIMEOUT(flag == true, 5000);
  });
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  flag = true;
  t3.join();
  waitUpdate();
}

BOOST_AUTO_TEST_CASE(TestWithSigusr1) {
  HelperTest test;
  test.testRoutine();
}
