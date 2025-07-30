cmake_minimum_required(VERSION 3.16)
PROJECT(doocs-server-test-helper)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 07)
set(${PROJECT_NAME}_PATCH_VERSION 02)
include(cmake/set_version_numbers.cmake)

include(cmake/add_linter_target.cmake)

# set c++ 11 support (or newer):
include(cmake/enable_latest_cxx_support.cmake)

# dependencies
find_package(DOOCS 24.03 COMPONENTS server REQUIRED)
find_package(Boost COMPONENTS unit_test_framework filesystem REQUIRED)
find_package(Threads)

# compiler flags etc.
include(cmake/set_default_flags.cmake)

# C++ library
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)
add_library(${PROJECT_NAME} SHARED ${library_sources})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
  SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"

  # relative to ${CMAKE_INSTALL_PREFIX} but don't explicitly mention it, to make result relocatable
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(${PROJECT_NAME} PUBLIC DOOCS::server Threads::Threads Boost::filesystem)

# Unit tests
enable_testing()
aux_source_directory(${CMAKE_SOURCE_DIR}/tests/executables_src testExecutables)

foreach(testExecutableSrcFile ${testExecutables})
  # NAME_WE means the base name without path and (longest) extension
  get_filename_component(excutableName ${testExecutableSrcFile} NAME_WE)
  add_executable(${excutableName} ${testExecutableSrcFile})
  target_link_libraries(${excutableName} ${PROJECT_NAME} Threads::Threads)
  add_test(${excutableName} ${excutableName})
endforeach(testExecutableSrcFile)

# generate Doxygen documentation
include(cmake/enable_doxygen_documentation.cmake)

# Install the library and the header files
# this defines architecture-dependent ${CMAKE_INSTALL_LIBDIR}
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/ DESTINATION include/${PROJECT_NAME})

# generate cmake config so other projects can find this library
# we need the public dependencies so create_cmake_config_files can find them as implicit dependencies
list(APPEND ${PROJECT_NAME}_PUBLIC_DEPENDENCIES "DOOCS COMPONENTS server;Threads")

# we support our cmake EXPORTS as imported targets
set(PROVIDES_EXPORTED_TARGETS 1)
include(${CMAKE_SOURCE_DIR}/cmake/create_cmake_config_files.cmake)
