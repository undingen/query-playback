cmake_minimum_required(VERSION 2.8)

add_subdirectory(test EXCLUDE_FROM_ALL)

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_library(libpercona-playbackplugin db_thread.cc)

add_library(libpercona-playback plugin.cc percona_playback.cc)

target_link_libraries(libpercona-playback "dl")

target_link_libraries(libpercona-playbackplugin ${Boost_LIBRARIES})

include_directories(${PROJECT_SOURCE_DIR})

## build plugins
set(plugin_list "")
set(plugin_sym_ist "")
file(GLOB_RECURSE plugins "${CMAKE_CURRENT_LIST_DIR}//*/plugin.ini")
FOREACH(plugin_cmakefile ${plugins})
  get_filename_component(plugin_path ${plugin_cmakefile} DIRECTORY)
  get_filename_component(plugin_name ${plugin_path} NAME)
  
  #if(NOT plugin_name STREQUAL "test" AND NOT plugin_path STREQUAL CMAKE_CURRENT_LIST_FILE)
  message("")
  message("Processing plugin ${plugin_name}:")
  add_subdirectory(${plugin_path})
  link_directories(${plugin_path})
  target_link_libraries(libpercona-playback ${plugin_name})
  list(APPEND plugin_list ${plugin_name})
  list(APPEND plugin_sym_list "_percona_playback_${plugin_name}_plugin_")
ENDFOREACH(plugin_cmakefile)

string(REPLACE ";" "," PANDORA_BUILTIN_LIST "${plugin_list}")
set(PANDORA_BUILTIN_LOAD_LIST ${PANDORA_BUILTIN_LIST})
string(REPLACE ";" "," PANDORA_BUILTIN_SYMBOLS_LIST "${plugin_sym_list}")
set(PANDORA_BUILTIN_LOAD_SYMBOLS_LIST ${PANDORA_BUILTIN_SYMBOLS_LIST})
configure_file("${CMAKE_CURRENT_LIST_DIR}/plugin_load_list.h.in" "${CMAKE_CURRENT_LIST_DIR}/plugin_load_list.h")
