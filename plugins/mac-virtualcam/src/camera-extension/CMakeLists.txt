foreach(_uuid IN ITEMS VIRTUALCAM_DEVICE_UUID VIRTUALCAM_SOURCE_UUID VIRTUALCAM_SINK_UUID)
  set(VALID_UUID FALSE)
  if(NOT ${_uuid})
    message(AUTHOR_WARNING "macOS Camera Extension UUID '${_uuid}' is not set, but required for extension.")
    return()
  else()
    check_uuid(${${_uuid}} VALID_UUID)

    if(NOT VALID_UUID)
      message(AUTHOR_WARNING "macos Camera Extension UUID '${_uuid}' is not a valid UUID.")
      return()
    endif()
  endif()
endforeach()

enable_language(Swift)

set(CMAKE_OSX_DEPLOYMENT_TARGET 12.3)#/PRISM/Zhongling/20231130/#/For MacOS 12.3

add_executable(mac-camera-extension)
add_executable(OBS:mac-camera-extension ALIAS mac-camera-extension)

set(_placeholder_location "${CMAKE_CURRENT_SOURCE_DIR}/../common/data/placeholder.png")
target_sources(
  mac-camera-extension PRIVATE "${_placeholder_location}" main.swift OBSCameraDeviceSource.swift
                               OBSCameraProviderSource.swift OBSCameraStreamSink.swift OBSCameraStreamSource.swift SwiftWebSocketServer.swift)

set_property(SOURCE "${_placeholder_location}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
source_group("Resources" FILES "${_placeholder_location}")

# cmake-format: off
set_target_properties_obs(
  mac-camera-extension
  PROPERTIES FOLDER plugins
             #/PRISM/Zhongling/20231123/#/New vcam start
             OUTPUT_NAME com.prismlive.prismlivestudio.mac-camera-extension
             #/PRISM/Zhongling/20231123/#/New vcam end
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
             MACOSX_BUNDLE ON
             MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
             BUNDLE_EXTENSION systemextension
             XCODE_PRODUCT_TYPE com.apple.product-type.system-extension)
  
string(TIMESTAMP CURRENT_YEAR "%Y")
if(NOT DEFINED ENV{PRISM_VCAM_BUNDLE_PP})
  message("NOT DEFINED PRISM_VCAM_BUNDLE_PP")
  set_target_xcode_properties(
  mac-camera-extension
  PROPERTIES SWIFT_VERSION 5.0
             MACOSX_DEPLOYMENT_TARGET 12.3 #/PRISM/Zhongling/20231130/#/For MacOS 12.3
             CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist"
             #/PRISM/Zhongling/20231123/#/New vcam start
             PRODUCT_NAME "com.prismlive.prismlivestudio.mac-camera-extension"
             PRODUCT_BUNDLE_IDENTIFIER "com.prismlive.prismlivestudio.mac-camera-extension"
             #/PRISM/Zhongling/20231123/#/New vcam end
             CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
             MARKETING_VERSION ${OBS_VERSION_CANONICAL}
             COPY_PHASE_STRIP NO
             GENERATE_INFOPLIST_FILE YES
             INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2022-${CURRENT_YEAR} Sebastian Beckmann, Patrick Heyer"
             #/PRISM/Zhongling/20231123/#/New vcam start
             INFOPLIST_KEY_NSSystemExtensionUsageDescription "This Camera Extension enables virtual camera functionality in PRISM Live Studio.")
             #/PRISM/Zhongling/20231123/#/New vcam start
  return()
endif()

message("DEFINED PRISM_VCAM_BUNDLE_PP ${PRISM_VCAM_BUNDLE_PP}")
set_target_xcode_properties(
  mac-camera-extension
  PROPERTIES SWIFT_VERSION 5.0
             MACOSX_DEPLOYMENT_TARGET 12.3 #/PRISM/Zhongling/20231130/#/For MacOS 12.3
             CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist"
             #/PRISM/Zhongling/20231123/#/New vcam start
             PRODUCT_NAME "com.prismlive.prismlivestudio.mac-camera-extension"
             PRODUCT_BUNDLE_IDENTIFIER "com.prismlive.prismlivestudio.mac-camera-extension"
             DEVELOPMENT_TEAM "K9UP278937"
             PROVISIONING_PROFILE_SPECIFIER "Prismlivestudio MacCameraExtension DeveloperID PP"
             CODE_SIGN_IDENTITY "Developer ID Application"
             #/PRISM/Zhongling/20231123/#/New vcam end
             CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
             MARKETING_VERSION ${OBS_VERSION_CANONICAL}
             COPY_PHASE_STRIP NO
             GENERATE_INFOPLIST_FILE YES
             INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2022-${CURRENT_YEAR} Sebastian Beckmann, Patrick Heyer"
             #/PRISM/Zhongling/20231123/#/New vcam start
             INFOPLIST_KEY_NSSystemExtensionUsageDescription "This Camera Extension enables virtual camera functionality in PRISM Live Studio.")
             #/PRISM/Zhongling/20231123/#/New vcam start
# cmake-format: on
