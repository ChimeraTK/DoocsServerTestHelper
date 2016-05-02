PROJECT(mtca4u-doocsServerTestHelper)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 00)
set(${PROJECT_NAME}_MINOR_VERSION 01)
set(${PROJECT_NAME}_PATCH_VERSION 00)
set(${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION}.${${PROJECT_NAME}_PATCH_VERSION})
set(${PROJECT_NAME}_SOVERSION ${${PROJECT_NAME}_MAJOR_VERSION}.${${PROJECT_NAME}_MINOR_VERSION})

#Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -ansi -Wuninitialized")

#use -DCMAKE_BUILD_TYPE=Debug in your cmake command to turn on the coverage option
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")

# set c++ 11 support:
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(${PROJECT_NAME}_CPPFLAGS "${${PROJECT_NAME}_CPPFLAGS} -std=c++11")
    set(${PROJECT_NAME}_CMAKE_CXX_FLAGS "${${PROJECT_NAME}_CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    set(${PROJECT_NAME}_CPPFLAGS "${${PROJECT_NAME}_CPPFLAGS} -std=c++0x")
    set(${PROJECT_NAME}_CMAKE_CXX_FLAGS "${${PROJECT_NAME}_CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed -Wl,--export-dynamic")

FIND_PACKAGE(DOOCS 18.10 REQUIRED)
include_directories(SYSTEM ${DOOCS_INCLUDE_DIRS})
link_directories(${DOOCS_LIBRARY_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)

aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${DOOCS_CXX_FLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed ${DOOCS_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${DOOCS_LIBRARIES})

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ALL
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/doc DESTINATION .)
else(DOXYGEN_FOUND)
  message("Doxygen not found, documentation will not be build.")
endif(DOXYGEN_FOUND)

#Install the library and the executables
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
	    LIBRARY DESTINATION lib
  )

#create the config files by filling the correct variables into the template (*.cmake.in)
#All variables @VARIABLE@ are replaced with the current values

#some variables we need for regular Makefiles, for which we also provide the config
set(${PROJECT_NAME}_CPPFLAGS "${${PROJECT_NAME}_CPPFLAGS} -I${CMAKE_INSTALL_PREFIX}/include ")
set(${PROJECT_NAME}_LDFLAGS "-L${CMAKE_INSTALL_PREFIX}/lib -Wl,--no-as-needed -l${PROJECT_NAME} ")
#MATLABs mex cannot deal with the rpath and C+=11 flags, so we provide special flags
set(${PROJECT_NAME}_MEXFLAGS "-I${CMAKE_INSTALL_PREFIX}/include ${${PROJECT_NAME}_LDFLAGS} ")
#only now we extend the LDFLAGS to its final value, including the runpath
set(${PROJECT_NAME}_LDFLAGS "${${PROJECT_NAME}_LDFLAGS} -Wl,-rpath=${CMAKE_INSTALL_PREFIX}/lib,--enable-new-dtags ")

# header file used to identify the presence of the package in a certain include directory
set(${PROJECT_NAME}_REFERENCE_HEADER "mtca4u-doocsServerTestHelper/doocsServerTestHelper.h")

#We have nested @-statements, so we have to parse twice
configure_file(cmake/PROJECT_NAMEConfig.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

configure_file(cmake/PROJECT_NAMEConfigVersion.cmake.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" @ONLY)
 
configure_file(cmake/PROJECT_NAME-config.in.in
  "${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in" @ONLY)
configure_file(${PROJECT_BINARY_DIR}/cmake/${PROJECT_NAME}-config.in
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config" @ONLY)

configure_file(cmake/FindPROJECT_NAME.cmake.in
  "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake" @ONLY)

# Install the ..Config.cmake and ..ConfigVersion.cmake
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" 
  "${PROJECT_BINARY_DIR}/Find${PROJECT_NAME}.cmake"
  DESTINATION share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules COMPONENT dev)

install(PROGRAMS "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config"
  DESTINATION bin COMPONENT dev)

#all include files go into include/PROJECT_NAME
#The exclusion of ${PROJECT_NAME} prevents the recursive installation of the files just being installed.
#The original headers are in include/*.h, the installed ones in include/PROJECT_NAME/*.h.
install(DIRECTORY ${${PROJECT_NAME}_INCLUDE_DIRS} DESTINATION include/${PROJECT_NAME}
          FILES_MATCHING PATTERN "*.h" 
	  PATTERN ".svn" EXCLUDE
	  PATTERN "${PROJECT_NAME}" EXCLUDE
	  )

# The instructions to create a debian package are loaded from a separate file
#include(${CMAKE_SOURCE_DIR}/cmake/prepare_debian_package.cmake)
