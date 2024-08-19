cmake_minimum_required(VERSION 3.24...3.25)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  add_library(obs-virtualcam-interface INTERFACE)
  add_library(OBS::virtualcam-interface ALIAS obs-virtualcam-interface)

  target_sources(obs-virtualcam-interface INTERFACE shared-memory-queue.c shared-memory-queue.h tiny-nv12-scale.c
                                                    tiny-nv12-scale.h)
  target_include_directories(obs-virtualcam-interface INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

  # PRISM/Xiewei/20231123/#2859/modify enumeration in libdshowcapture to avoid device block. start
  # include(cmake/libdshowcapture.cmake)
  include(cmake/pls-libdshowcapture.cmake)
  # PRISM/Xiewei/20231123/#2859/modify enumeration in libdshowcapture to avoid device block. end
  add_subdirectory(virtualcam-module)
endif()

legacy_check()

find_package(FFmpeg REQUIRED avcodec avutil)

add_library(win-dshow MODULE)
add_library(OBS::dshow ALIAS win-dshow)

target_sources(
  win-dshow
  PRIVATE # cmake-format: sortable
          dshow-plugin.cpp encode-dstr.hpp ffmpeg-decode.c ffmpeg-decode.h win-dshow-encoder.cpp win-dshow.cpp)

# PRISM/Xiewei/20240625/VOC/add log and trace video data start
include(cmake/pls-files.cmake)
# PRISM/Xiewei/20240625/VOC/add log and trace video data end

configure_file(cmake/windows/obs-module.rc.in win-dshow.rc)
target_sources(win-dshow PRIVATE win-dshow.rc)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
target_link_libraries(
  win-dshow
  PRIVATE OBS::libobs
          OBS::w32-pthreads
          OBS::libdshowcapture
          FFmpeg::avcodec
          FFmpeg::avutil
          strmiids
          winmm)
endif()

if(TARGET OBS::virtualcam AND TARGET OBS::virtualcam-guid)
  target_sources(win-dshow PRIVATE virtualcam.c)
  target_link_libraries(win-dshow PRIVATE OBS::virtualcam-interface OBS::virtualcam-guid)
  target_compile_definitions(win-dshow PRIVATE VIRTUALCAM_AVAILABLE)

  add_dependencies(win-dshow obs-virtualcam-module)
endif()

# cmake-format: off
set_target_properties_obs(win-dshow PROPERTIES FOLDER plugins/win-dshow PREFIX "")
# cmake-format: on
