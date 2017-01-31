LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

read_plugin_ini("general_log")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_library(general_log parse_general_log.cc general_log_entry.cc general_log.cc)
target_link_libraries(general_log ${Boost_LIBRARIES})

install(TARGETS general_log DESTINATION lib)

