# search path for capnp schemata: *.capnp
set(RIME_PROTO_SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
# search path for generated capnp binding headers: *.capnp.h
set(RIME_PROTO_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR} PARENT_SCOPE)

capnp_generate_cpp(
  CAPNP_SRCS
  CAPNP_HDRS
  rime_proto.capnp
)
message(STATUS "compiled capnproto schemata (sources): ${CAPNP_SRCS}")
message(STATUS "compiled capnproto schemata (headers): ${CAPNP_HDRS}")

add_library(rime-proto-objs OBJECT ${CAPNP_SRCS})
target_include_directories(rime-proto-objs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
if(BUILD_SHARED_LIBS)
set_target_properties(rime-proto-objs
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON)
endif()

set(rime_proto_objs $<TARGET_OBJECTS:rime-proto-objs> PARENT_SCOPE)
