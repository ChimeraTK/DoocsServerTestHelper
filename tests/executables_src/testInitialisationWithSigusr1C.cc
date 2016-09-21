#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // start the initialisation in the background. it cannot complete since we do not yet enter nanosleep in the update thread)
  std::thread t([](){
    std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
    DoocsServerTestHelper::initialise();
    std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;
  });

  // wait some time, then enter nanosleep in the update thread
  usleep(100000);
  allowUpdate();

  // initialise will execute update once, so wait until nanosleep has finished the first time
  waitUpdate();

  // now let the update thread enter nanosleep another time, so initialise() can complete
  allowUpdate();

  // let the thread execute sigwait once (this call to sigwait will be the normal sigwait receiving the SIGUSR1 from initialise())
  allowSigusr1();

  // now the initialisation must be able to complete
  t.join();

  // test a full update cycle
  allowUpdate();
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  waitUpdate();

  // test a full sigusr1 cycle
  allowSigusr1();
  allowSigusr1();   // twice since the tread needs to enter sigwait() again before runSigusr1() returns
  std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
  DoocsServerTestHelper::runSigusr1();
  std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
  waitSigusr1();

}
