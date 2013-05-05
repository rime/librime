set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. LIBRIME_SRC_BASE)
aux_source_directory(algo LIBRIME_SRC_ALGO)
aux_source_directory(dict LIBRIME_SRC_DICT)
aux_source_directory(gear LIBRIME_SRC_GEAR)
aux_source_directory(lever LIBRIME_SRC_LEVER)

set(LIBRIME_SRC ${LIBRIME_SRC_BASE} ${LIBRIME_SRC_ALGO} ${LIBRIME_SRC_DICT} ${LIBRIME_SRC_GEAR} ${LIBRIME_SRC_LEVER})

set(RIME_OPTIONAL_DEPS "")
if(Gflags_FOUND)
  set(RIME_OPTIONAL_DEPS ${RIME_OPTIONAL_DEPS} ${Gflags_LIBRARY})
endif(Gflags_FOUND)

set(RIME_DEPS
    ${Boost_LIBRARIES} ${Glog_LIBRARY} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${Opencc_LIBRARY}
    ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${RIME_OPTIONAL_DEPS})

if(NOT BUILD_SHARED_LIBS)
  add_library(rime-static STATIC ${LIBRIME_SRC})
  target_link_libraries(rime-static ${RIME_DEPS})
  set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")
  install(TARGETS rime-static DESTINATION ${LIB_INSTALL_DIR})
else(NOT BUILD_SHARED_LIBS)
  add_library(rime ${LIBRIME_SRC})
  target_link_libraries(rime ${RIME_DEPS})
  set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
  set_target_properties(rime PROPERTIES VERSION ${LIBRIME_VERSION} SOVERSION ${LIBRIME_SOVERSION})
  if(XCODE_VERSION)
    set_target_properties(rime PROPERTIES INSTALL_NAME_DIR "@rpath")
  endif(XCODE_VERSION)
  install(TARGETS rime DESTINATION ${LIB_INSTALL_DIR})
endif(NOT BUILD_SHARED_LIBS)
