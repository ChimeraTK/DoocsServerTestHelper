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
  std::atomic<bool> flag{}, flag2{};
  std::cout << "HelperTest::testRoutineBody() 1" << std::endl;

  // allow the update thread to enter nanosleep and the sigusr1 thread to enter
  // sigwait
  allowUpdate();
  allowSigusr1();

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

  // test a full sigusr1 cycle
  flag = false;
  std::thread t2([&flag] {
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  CHECK_TIMEOUT(flag == true, 5000);
  t2.join();
  waitSigusr1();

  // test a second full update cycle
  flag = false;
  std::thread t3([&flag] {
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowUpdate();
  CHECK_TIMEOUT(flag == true, 5000);
  t3.join();
  waitUpdate();

  // test running both at the "same" time
  flag = false;
  flag2 = false;
  std::thread t4([&flag] {
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  std::thread t5([&flag2] {
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag2 = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  CHECK_TIMEOUT(flag == true, 5000);
  t4.join();
  waitSigusr1();
  usleep(100000);
  BOOST_CHECK(flag2 == false);
  allowUpdate();
  CHECK_TIMEOUT(flag2 == true, 5000);
  t5.join();
  waitUpdate();

  // test running both at the "same" time (different order)
  flag = false;
  flag2 = false;
  std::thread t6([&flag] {
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  std::thread t7([&flag2] {
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag2 = true;
  });
  usleep(100000);
  BOOST_CHECK(flag2 == false);
  allowUpdate();
  CHECK_TIMEOUT(flag2 == true, 5000);
  t7.join();
  waitUpdate();
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  CHECK_TIMEOUT(flag == true, 5000);
  t6.join();
  waitSigusr1();
}

BOOST_AUTO_TEST_CASE(testWithSigusr1) {
  HelperTest test;
  test.testRoutine();
}