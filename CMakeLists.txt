cmake_minimum_required(VERSION 2.8)

enable_testing()
add_subdirectory(percona_playback)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 7)
set(PATCH_VERSION 0)

set(playback_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(playback_PACKAGE "percona-playback")
set(playback_BUGREPORT "https://bugs.launchpad.net/percona-playback")

configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_SOURCE_DIR}/config.h")

add_executable(percona-playback bin/percona_playback.cc)

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_BINARY_DIR}/percona_playback")
#target_link_libraries(percona-playback "dl")
#target_link_libraries(percona-playback ${Boost_LIBRARIES})
#target_link_libraries(percona-playback "tbb")
target_link_libraries(percona-playback libpercona-playback)

install(TARGETS percona-playback RUNTIME DESTINATION bin LIBRARY DESTINATION lib)


#add_test(test-basic-multiline ${PROJECT_BINARY_DIR}/percona_playback/test/test-basic-multiline)

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
