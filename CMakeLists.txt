cmake_minimum_required(VERSION 2.8)

project(percona-playback)
add_subdirectory(percona_playback/test EXCLUDE_FROM_ALL)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 7)
set(PATCH_VERSION 0)

set(playback_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(playback_PACKAGE "percona-playback")
set(playback_BUGREPORT "https://bugs.launchpad.net/percona-playback")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_SOURCE_DIR}/config.h")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

add_executable(percona-playback percona_playback/db_thread.cc
                                percona_playback/percona_playback.cc
                                percona_playback/plugin.cc
                                bin/percona_playback.cc)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}/percona_playback")
target_link_libraries(percona-playback "dl")
target_link_libraries(percona-playback ${Boost_LIBRARIES})
target_link_libraries(percona-playback "tbb")

install(TARGETS percona-playback RUNTIME DESTINATION bin LIBRARY DESTINATION lib)

enable_testing()
add_test(test-multiline $<TARGET_FILE:test-multiline>)

set(CPACK_DEBIAN_PACKAGE_DEPENDS "tbb mysql")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Percona-Lab/query-playback")
set(CPACK_DEBIAN_PACKAGE_SECTION "Applications/Databases")
set(CPACK_PACKAGE_CONTACT "https://github.com/Percona-Lab/query-playback")
set(CPACK_PACKAGE_DESCRIPTION "Percona Playback is a tool for replaying the captured load of one database "
                              "server against another in the most realistic way possible. Captured load can "
                              "come in the form of MySQL query logs.  It's multithreaded, modular and "
                              "configurable to allow for flexibility and future extension.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for replaying captured database server load")
set(CPACK_PACKAGE_VENDOR "Percona Lab")
set(CPACK_PACKAGE_VERSION_MAJOR "${MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${MINOR_VERSION}")
set(CPACK_PACKAGE_VERSION_PATCH "${PATCH_VERSION}")
include(CPack)

## build plugins
set(plugin_list "")
set(plugin_sym_ist "")
file(GLOB_RECURSE plugins "percona_playback/*/CMakeLists.txt")
FOREACH(plugin_cmakefile ${plugins})
  get_filename_component(plugin_path ${plugin_cmakefile} DIRECTORY)
  get_filename_component(plugin_name ${plugin_path} NAME)
  
  if(NOT plugin_name STREQUAL "test")
    message("")
    message("Processing plugin ${plugin_name}:")
    add_subdirectory(${plugin_path})
    link_directories(${plugin_path})
    target_link_libraries(percona-playback ${plugin_name})
    list(APPEND plugin_list ${plugin_name})
    list(APPEND plugin_sym_list "_percona_playback_${plugin_name}_plugin_")
  endif(NOT plugin_name STREQUAL "test")
ENDFOREACH(plugin_cmakefile)

string(REPLACE ";" "," PANDORA_BUILTIN_LIST "${plugin_list}")
set(PANDORA_BUILTIN_LOAD_LIST ${PANDORA_BUILTIN_LIST})
string(REPLACE ";" "," PANDORA_BUILTIN_SYMBOLS_LIST "${plugin_sym_list}")
set(PANDORA_BUILTIN_LOAD_SYMBOLS_LIST ${PANDORA_BUILTIN_SYMBOLS_LIST})
configure_file("${PROJECT_SOURCE_DIR}/percona_playback/plugin_load_list.h.in" "${PROJECT_SOURCE_DIR}/percona_playback/plugin_load_list.h")


