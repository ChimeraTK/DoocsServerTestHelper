#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // let the thread execute nanosleep three times (it will stay in the third call after initialise())
  allowUpdate();    // the first nanosleep() will be entered before initialise() is called and thus will just sleep 1 second
  allowUpdate();    // the second nanosleep() is triggered by the runUpdate() inside initialise()
  allowUpdate();    // the third nanosleep() is entered in/right after initialise() and awaits the next runUpdate()
  usleep(100000);   // make sure we entered the first nanosleep before the call to initialise()

  // now initialise the timing system
  std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
  DoocsServerTestHelper::initialise();
  std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;

  // check that nanosleep has been executed twice now
  waitUpdate();
  waitUpdate();

  // test a full update cycle
  allowUpdate();
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  waitUpdate();

}
