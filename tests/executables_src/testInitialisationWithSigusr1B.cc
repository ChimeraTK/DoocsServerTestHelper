#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // let the thread execute sigwait once (it shall be in this first call before initialise() and will stay there after initialise())
  allowSigusr1();
  sleep(1);

  // let the thread execute nanosleep three times (it will stay in the third call after initialise())
  allowUpdate();    // the first nanosleep() will be entered before initialise() is called and thus will just sleep 1 second
  allowUpdate();    // the second nanosleep() is triggered by the runUpdate() inside initialise()
  allowUpdate();    // the third nanosleep() is entered in/right after initialise() and awaits the next runUpdate()
  usleep(100000);   // make sure we entered the first nanosleep before the call to initialise()

  // now initialise the timing system
  std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
  DoocsServerTestHelper::initialise();
  std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;

  // initialise() has sent SIGUSR1 to leave the normal sigwait(), so we need to allow entering the next sigwait()
  waitSigusr1();
  allowSigusr1();

  // check that nanosleep has been executed twice now
  waitUpdate();
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
