LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

read_plugin_ini("simple_report")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_library(simple_report simple_report.cc)
target_link_libraries(simple_report ${Boost_LIBRARIES})

