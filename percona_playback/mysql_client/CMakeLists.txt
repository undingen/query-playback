LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/percona_playback")
include(plugin)

read_plugin_ini("mysql_client")

find_package(Boost COMPONENTS system thread regex program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

#exec_program(mysql_config ARGS --cflags OUTPUT_VARIABLE MYSQL_CFLAGS)
#exec_program(mysql_config ARGS --libs_r OUTPUT_VARIABLE MYSQL_LIBS)
#add_definitions(${MYSQL_CFLAGS})

find_path(MYSQL_INC "mysql/mysql.h")
include_directories("${MYSQL_INC}/mysql")

add_library(mysql_client mysql_client.cc)
target_link_libraries(mysql_client ${Boost_LIBRARIES})
target_link_libraries(mysql_client mysqlclient_r)

#set_target_properties(mysql_client PROPERTIES LINK_FLAGS ${MYSQL_LIBS})
#
#
#strings(REGEX MATCHALL "-l[\\W*]", 
#
#add_compile_options(${MYSQL_LIBS})
