function(read_plugin_ini plugin_name)
 file(STRINGS plugin.ini ini_text NEWLINE_CONSUME)

 STRING(REGEX REPLACE "(.*)title=([^\n]+)(.*)" "\\2" plugin_title "${ini_text}")
 STRING(REGEX REPLACE "(.*)description=([^\n]+)(.*)" "\\2" description "${ini_text}")
 STRING(REGEX REPLACE "(.*)version=([^\n]+)(.*)" "\\2" plugin_version "${ini_text}")
 STRING(REGEX REPLACE "(.*)author=([^\n]+)(.*)" "\\2" plugin_author "${ini_text}")

 #message(${plugin_title})
 #message(${description})
 #message(${plugin_version})
 #message(${plugin_author})
 #message(${ini_text})

 include_directories("${CMAKE_SOURCE_DIR}")
 include_directories("${CMAKE_SOURCE_DIR}/percona_playback")

 add_definitions(-DPANDORA_MODULE_VERSION="${plugin_version}")
 add_definitions(-DPANDORA_MODULE_AUTHOR="")
 add_definitions(-DPANDORA_MODULE_TITLE="${plugin_title}")
 add_definitions(-DPANDORA_MODULE_NAME=${plugin_name})
endfunction(read_plugin_ini)

