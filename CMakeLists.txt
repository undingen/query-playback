
project(percona-playback)

find_package(Boost COMPONENTS system threads REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

include_directories("${PROJECT_BINARY_DIR}/percona_playback")
add_executable(percona-playback percona_playback/plugin.cc percona_playback/percona_playback.cc percona_playback/db_thread.cc bin/percona_playback.cc)
