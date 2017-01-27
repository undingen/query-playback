cmake_minimum_required(VERSION 2.8)

project(percona-playback)

set(playback_VERSION "0.7")
set(playback_PACKAGE "percona-playback")
set(playback_BUGREPORT "https://bugs.launchpad.net/percona-playback")


configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_SOURCE_DIR}/config.h")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

include_directories("${PROJECT_BINARY_DIR}/percona_playback")
include_directories("${PROJECT_SOURCE_DIR}")
add_executable(percona-playback percona_playback/plugin.cc percona_playback/percona_playback.cc percona_playback/db_thread.cc bin/percona_playback.cc)
target_link_libraries(percona-playback "dl")
target_link_libraries(percona-playback ${Boost_LIBRARIES})
