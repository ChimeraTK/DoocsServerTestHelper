#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {
  std::atomic<bool> flag;
  
  // check if nanosleep with different time than the magic number works
  struct timespec nonMagicSleepTime{0, 10};
  nanosleep(&nonMagicSleepTime, NULL);

  // allow the update thread to enter nanosleep
  allowUpdate();

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

  // test a second full update cycle
  flag = false;
  std::thread t2([&flag ]{
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
  t2.join();
  waitUpdate();
  
  // check if nanosleep with different time than the magic number works
  struct timespec nonMagicSleepTime2{1, 0};
  nanosleep(&nonMagicSleepTime2, NULL);

}
