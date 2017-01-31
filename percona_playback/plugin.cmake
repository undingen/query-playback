function(read_ini_key ini_text key outputvar)
 STRING(REGEX REPLACE "(.*)${key}=([^\n]+)(.*)" "\\2" outputvar "${ini_text}")
endfunction(read_ini_key)

function(read_plugin_ini plugin_name)
 file(STRINGS plugin.ini ini_text NEWLINE_CONSUME)

 read_ini_key(${ini_text} "title" plugin_title)
 read_ini_key(${ini_text} "description" plugin_description)
 read_ini_key(${ini_text} "version" plugin_version)
 read_ini_key(${ini_text} "author" plugin_author)
 #STRING(REGEX REPLACE "(.*)title=([^\n]+)(.*)" "\\2" plugin_title "${ini_text}")
 #STRING(REGEX REPLACE "(.*)description=([^\n]+)(.*)" "\\2" description "${ini_text}")
 #STRING(REGEX REPLACE "(.*)version=([^\n]+)(.*)" "\\2" plugin_version "${ini_text}")
 #STRING(REGEX REPLACE "(.*)author=([^\n]+)(.*)" "\\2" plugin_author "${ini_text}")

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


 #add_definitions(-DBUILDING_PERCONA_PLAYBACK)

 target_link_libraries(${plugin_name} libpercona-playbackplugin)
endfunction(read_plugin_ini)

