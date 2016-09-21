#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // let the thread execute nanosleep twice (once it will be completed inside initialise(), after which it will stay in the second call)
  allowUpdate();
  allowUpdate();

  // now initialise the timing system
  std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
  DoocsServerTestHelper::initialise();
  std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;

  // let the thread execute sigwait once (it will stay in this first call after initialise())
  allowSigusr1();

  // check that nanosleep has been executed once now
  waitUpdate();

  // test a full sigusr1 cycle
  allowSigusr1();
  std::cout << "DoocsServerTestHelper::runSigusr1() ->" << std::endl;
  DoocsServerTestHelper::runSigusr1();
  std::cout << "<- DoocsServerTestHelper::runSigusr1()" << std::endl;
  waitSigusr1();

  // test a full update cycle
  allowUpdate();
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  waitUpdate();

}
