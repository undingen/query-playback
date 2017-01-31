LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

read_plugin_ini("thread_per_connection")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_library(thread_per_connection thread_per_connection.cc)
target_link_libraries(thread_per_connection ${Boost_LIBRARIES})
target_link_libraries(thread_per_connection tbb)
