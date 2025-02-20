add_library(libobs-version OBJECT)
add_library(OBS::libobs-version ALIAS libobs-version)

#PRISM/wangshaohui/20240905/#3138/ensure PLS_VERSION is defined
if(NOT DEFINED PLS_VERSION OR "${PLS_VERSION}" STREQUAL "")
	message(FATAL_ERROR "ERROR: PLS_VERSION is not defined or it is empty!")
else()
	message(STATUS "INFO: PLS_VERSION is defined!")
endif()

configure_file(obsversion.c.in obsversion.c @ONLY)

target_sources(
  libobs-version
  PRIVATE obsversion.c
  PUBLIC obsversion.h)

target_include_directories(libobs-version PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

set_property(TARGET libobs-version PROPERTY FOLDER core)
