file(GLOB_RECURSE T_SOURCES *.c)

idf_component_register(SRCS ${T_SOURCES} INCLUDE_DIRS "." REQUIRES "driver" "nofrendo" "hid" "ws_7inch")
