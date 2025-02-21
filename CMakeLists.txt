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
         BASE_DIRS src
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
  include(CMakePackageConfigHelpers)

  # Configure version and config files
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/parking-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/parking-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/parking-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/parking")

  # Install targets and config files
  install(
    TARGETS parking
    EXPORT parking-targets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    FILE_SET CXX_MODULES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/parking")

  install(
    EXPORT parking-targets
    FILE parking-targets.cmake
    NAMESPACE parking::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/parking")

  install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/parking-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/parking-config-version.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/parking")
endif()

# Testing configuration
if(PROJECT_IS_TOP_LEVEL)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
