#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {
  std::atomic<bool> flag, flag2;
  std::cout << "HelperTest::testRoutineBody() 1" << std::endl;

  // allow the update thread to enter nanosleep and the sigusr1 thread to enter sigwait
  allowUpdate();
  allowSigusr1();

  // test a full update cycle
  flag = false;
  std::thread t1([&flag ]{
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowUpdate();
  usleep(100000);
  BOOST_CHECK(flag == true);
  t1.join();
  waitUpdate();

  // test a full sigusr1 cycle
  flag = false;
  std::thread t2([&flag ]{
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  usleep(100000);
  BOOST_CHECK(flag == true);
  t2.join();
  waitSigusr1();

  // test a second full update cycle
  flag = false;
  std::thread t3([&flag ]{
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowUpdate();
  usleep(100000);
  BOOST_CHECK(flag == true);
  t3.join();
  waitUpdate();

  // test running both at the "same" time
  flag = false;
  flag2 = false;
  std::thread t4([&flag ]{
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  std::thread t5([&flag2 ]{
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag2 = true;
  });
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  usleep(100000);
  BOOST_CHECK(flag == true);
  t4.join();
  waitSigusr1();
  usleep(100000);
  BOOST_CHECK(flag2 == false);
  allowUpdate();
  usleep(100000);
  BOOST_CHECK(flag2 == true);
  t5.join();
  waitUpdate();

  // test running both at the "same" time (different order)
  flag = false;
  flag2 = false;
  std::thread t6([&flag ]{
    std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
    DoocsServerTestHelper::runSigusr1();
    std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
    flag = true;
  });
  std::thread t7([&flag2 ]{
    std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
    DoocsServerTestHelper::runUpdate();
    std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
    flag2 = true;
  });
  usleep(100000);
  BOOST_CHECK(flag2 == false);
  allowUpdate();
  usleep(100000);
  BOOST_CHECK(flag2 == true);
  t7.join();
  waitUpdate();
  usleep(100000);
  BOOST_CHECK(flag == false);
  allowSigusr1();
  usleep(100000);
  BOOST_CHECK(flag == true);
  t6.join();
  waitSigusr1();

}
