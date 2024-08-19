add_library(libdshowcapture INTERFACE)
add_library(OBS::libdshowcapture ALIAS libdshowcapture)

target_sources(
  libdshowcapture
  INTERFACE pls/libdshowcapture/dshowcapture.hpp
            pls/libdshowcapture/source/capture-filter.cpp
            pls/libdshowcapture/source/capture-filter.hpp
            pls/libdshowcapture/source/device-vendor.cpp
            pls/libdshowcapture/source/device.cpp
            pls/libdshowcapture/source/device.hpp
            pls/libdshowcapture/source/dshow-base.cpp
            pls/libdshowcapture/source/dshow-base.hpp
            pls/libdshowcapture/source/dshow-demux.cpp
            pls/libdshowcapture/source/dshow-demux.hpp
            pls/libdshowcapture/source/dshow-device-defs.hpp
            pls/libdshowcapture/source/dshow-encoded-device.cpp
            pls/libdshowcapture/source/dshow-enum.cpp
            pls/libdshowcapture/source/dshow-enum.hpp
            pls/libdshowcapture/source/dshow-formats.cpp
            pls/libdshowcapture/source/dshow-formats.hpp
            pls/libdshowcapture/source/dshow-media-type.cpp
            pls/libdshowcapture/source/dshow-media-type.hpp
            pls/libdshowcapture/source/dshowcapture.cpp
            pls/libdshowcapture/source/dshowencode.cpp
            pls/libdshowcapture/source/encoder.cpp
            pls/libdshowcapture/source/encoder.hpp
            pls/libdshowcapture/source/external/IVideoCaptureFilter.h
            pls/libdshowcapture/source/log.cpp
            pls/libdshowcapture/source/log.hpp
            pls/libdshowcapture/source/output-filter.cpp
            pls/libdshowcapture/source/output-filter.hpp
            pls/libdshowcapture/external/capture-device-support/Library/EGAVResult.cpp
            pls/libdshowcapture/external/capture-device-support/Library/ElgatoUVCDevice.cpp
            pls/libdshowcapture/external/capture-device-support/Library/win/EGAVHIDImplementation.cpp
            pls/libdshowcapture/external/capture-device-support/SampleCode/DriverInterface.cpp)

target_include_directories(
  libdshowcapture INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/pls/libdshowcapture"
                            "${CMAKE_CURRENT_SOURCE_DIR}/pls/libdshowcapture/external/capture-device-support/Library")

target_compile_definitions(libdshowcapture INTERFACE _UP_WINDOWS=1)
target_compile_options(libdshowcapture INTERFACE /wd4018)

get_target_property(target_sources libdshowcapture INTERFACE_SOURCES)
set(target_headers ${target_sources})
set(target_external_sources ${target_sources})

list(FILTER target_external_sources INCLUDE REGEX ".+external/.+/.+\\.cpp")
list(FILTER target_sources EXCLUDE REGEX ".+external/.+/.+\\.cpp")
list(FILTER target_sources INCLUDE REGEX ".*\\.(m|c[cp]?p?|swift)")
list(FILTER target_headers INCLUDE REGEX ".*\\.h(pp)?")

source_group("libdshowcapture-external\\Source Files" FILES ${target_external_sources})
source_group("libdshowcapture\\Source Files" FILES ${target_sources})
source_group("libdshowcapture\\Header Files" FILES ${target_headers})
