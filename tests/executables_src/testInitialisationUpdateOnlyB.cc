#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // start the initialisation in the background. it cannot complete since we do not yet enter nanosleep in the update thread)
  std::thread t([](){
    std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
    DoocsServerTestHelper::initialise(false,false);
    std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;
  });

  // wait some time, then enter nanosleep in the update thread
  usleep(100000);
  allowUpdate();

  // initialise will execute update once, so wait until nanosleep has finished the first time
  waitUpdate();

  // now let the update thread enter nanosleep another time, so initialise() can complete
  allowUpdate();

  // now the initialisation must be able to complete
  t.join();

  // test a full update cycle
  allowUpdate();
  std::cout << "DoocsServerTestHelper::runUpdate() ->" << std::endl;
  DoocsServerTestHelper::runUpdate();
  std::cout << "<- DoocsServerTestHelper::runUpdate()" << std::endl;
  waitUpdate();

}
