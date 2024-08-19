file(GLOB_RECURSE pls-data-monitor-SRC_FILES pls/data-monitor/*.c pls/data-monitor/*.cpp pls/data-monitor/*.cxx pls/data-monitor/*.cc pls/data-monitor/*.m)
file(GLOB_RECURSE pls-data-monitor-HDR_FILES pls/data-monitor/*.h pls/data-monitor/*.hpp pls/data-monitor/*.hxx pls/data-monitor/*.hh)

target_sources(
  win-dshow
  PRIVATE
  ${pls-data-monitor-SRC_FILES}
  ${pls-data-monitor-HDR_FILES}
)

source_group("pls\\Header Files" FILES ${pls-data-monitor-HDR_FILES})
source_group("pls\\Source Files" FILES ${pls-data-monitor-SRC_FILES})
