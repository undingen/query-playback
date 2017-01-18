list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

add_library(simple_report simple_report.cc)
target_link_libraries(simple_report tbb)

setup_plugin("simple_report")
