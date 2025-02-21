cmake_minimum_required(VERSION 3.28)
project(
  parking
  VERSION 1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(parking src/impl/futex.cpp src/impl/pthread.cpp
                    src/pal/unix/futex.cpp src/pal/windows/futex.cpp)
add_library(parking::parking ALIAS parking)
target_sources(
  parking
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         src/mod.ixx
         src/pal/mod.ixx
         src/pthread.ixx
         src/futex.ixx
         src/pal/unix/futex.ixx
         src/pal/windows/futex.ixx)

# Add installation rules
if(PROJECT_IS_TOP_LEVEL)
  include(GNUInstallDirs)

  install(
    TARGETS parking
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# Testing configuration
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
