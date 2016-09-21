#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // let the thread execute nanosleep twice (once it will be completed inside initialise(), after which it will stay in the second call)
  allowUpdate();
  allowUpdate();

  // now initialise the timing system
  std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
  DoocsServerTestHelper::initialise();
  std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;

  // check that nanosleep has been executed once now
  waitUpdate();

  // test a full update cycle
  allowUpdate();
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  waitUpdate();

}
