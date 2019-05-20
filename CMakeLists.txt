PROJECT(doocs-server-test-helper)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 03)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

# set c++ 11 support (or newer):
include(cmake/enable_latest_cxx_support.cmake)

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-as-needed -Wl,--export-dynamic")

# dependencies
include(cmake/add_dependency.cmake)
add_dependency(DOOCS 19.5 server REQUIRED)

find_package(Boost COMPONENTS unit_test_framework REQUIRED)

# compiler flags etc.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -Wuninitialized")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# C++ library
add_library(${PROJECT_NAME} SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION} SOVERSION ${${PROJECT_NAME}_SOVERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS ${DOOCS_CXX_FLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed ${DOOCS_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${DOOCS_LIBRARIES})

# Unit tests
ENABLE_TESTING()
aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)
foreach( testExecutableSrcFile ${testExecutables})
  #NAME_WE means the base name without path and (longest) extension
  get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${excutableName} ${testExecutableSrcFile})
  target_link_libraries(${excutableName} ${PROJECT_NAME})
  add_test(${excutableName} ${excutableName})
endforeach( testExecutableSrcFile )

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

#Install the library and the header files
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME})

# generate cmake config so other projects can find this library
set(${PROJECT_NAME}_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/include ${DOOCS_INCLUDE_DIRS}")
set(${PROJECT_NAME}_LIBRARIES "${PROJECT_NAME} ${DOOCS_LIBRARIES}")
set(${PROJECT_NAME}_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib ${DOOCS_LIBRARY_DIRS}")
set(${PROJECT_NAME}_CXX_FLAGS "${DOOCS_CXX_FLAGS}")
set(${PROJECT_NAME}_LINKER_FLAGS "${DOOCS_LINK_FLAGS}")
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
