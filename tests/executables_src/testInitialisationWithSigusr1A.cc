#include "testDoocsServerTestHelper_skeleton.h"

void HelperTest::testRoutineBody() {

  // let the thread execute nanosleep three times (it will stay in the third call after initialise())
  allowUpdate();    // the first nanosleep() will be entered before initialise() is called and thus will just sleep 1 second
  allowUpdate();    // the second nanosleep() is triggered by the runUpdate() inside initialise()
  allowUpdate();    // the third nanosleep() is entered in/right after initialise() and awaits the next runUpdate()
  usleep(100000);   // make sure we entered the first nanosleep before the call to initialise()

  // start the initialisation in the background. it cannot complete since we do not yet enter sigwait in the sigusr1 thread)
  std::thread t([](){
    std::cout << "DoocsServerTestHelper::initialise() ->" << std::endl;
    DoocsServerTestHelper::initialise();
    std::cout << "<- DoocsServerTestHelper::initialise()" << std::endl;
  });

  // let the thread execute sigwait once (this call to sigwait will be the normal sigwait receiving the SIGUSR1 from initialise())
  allowSigusr1();

  // let the thread execute sigwait a second time (will stay in this sigwait after initialise())
  allowSigusr1();

  // now the initialisation must be able to complete
  t.join();

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
