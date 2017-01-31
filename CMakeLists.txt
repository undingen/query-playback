cmake_minimum_required(VERSION 2.8)

project(percona-playback)

set(playback_VERSION "0.7")
set(playback_PACKAGE "percona-playback")
set(playback_BUGREPORT "https://bugs.launchpad.net/percona-playback")


configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_SOURCE_DIR}/config.h")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

file(GLOB_RECURSE plugins "percona_playback/*/CMakeLists.txt")

add_executable(percona-playback percona_playback/plugin.cc
                                percona_playback/percona_playback.cc
                                percona_playback/db_thread.cc
                                bin/percona_playback.cc)

set(plugin_list "")
set(plugin_sym_ist "")
FOREACH(plugin_cmake ${plugins})
  get_filename_component(plugin_full ${plugin_cmake} DIRECTORY)
  get_filename_component(plugin ${plugin_full} NAME)
  
  message("")
  message("Processing plugin ${plugin}:")
  add_subdirectory(${plugin_full})
  link_directories("${CMAKE_BINARY_DIR}/percona_playback/${plugin}")
  target_link_libraries(percona-playback ${plugin})
  list(APPEND plugin_list ${plugin})
  list(APPEND plugin_sym_list "_percona_playback_${plugin}_plugin_")
ENDFOREACH(plugin_cmake)


string(REPLACE ";" "," PANDORA_BUILTIN_LIST "${plugin_list}")
set(PANDORA_BUILTIN_LOAD_LIST ${PANDORA_BUILTIN_LIST})
string(REPLACE ";" "," PANDORA_BUILTIN_SYMBOLS_LIST "${plugin_sym_list}")
set(PANDORA_BUILTIN_LOAD_SYMBOLS_LIST ${PANDORA_BUILTIN_SYMBOLS_LIST})
configure_file("${PROJECT_SOURCE_DIR}/percona_playback/plugin_load_list.h.in" "${PROJECT_SOURCE_DIR}/percona_playback/plugin_load_list.h")

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}/percona_playback")
target_link_libraries(percona-playback "dl")
target_link_libraries(percona-playback ${Boost_LIBRARIES})
target_link_libraries(percona-playback "tbb")

install(TARGETS percona-playback RUNTIME DESTINATION bin LIBRARY DESTINATION lib)

