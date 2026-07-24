file(GLOB_RECURSE pls-files pls/*.*)

target_sources(
  win-dshow
  PRIVATE
  ${pls-files}
)

source_group("pls" FILES ${pls-files})
