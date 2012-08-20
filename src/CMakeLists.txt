set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. LIBRIME_SRC)
aux_source_directory(algo LIBRIME_ALGO_SRC)
aux_source_directory(dict LIBRIME_DICT_SRC)
aux_source_directory(impl LIBRIME_IMPL_SRC)
aux_source_directory(expl LIBRIME_EXPL_SRC)

set(RIME_OPTIONAL_DEPS "")
if(Gflags_FOUND)
  set(RIME_OPTIONAL_DEPS ${RIME_OPTIONAL_DEPS} ${Gflags_LIBRARY})
endif(Gflags_FOUND)

add_library(rime-static STATIC ${LIBRIME_SRC} ${LIBRIME_ALGO_SRC} ${LIBRIME_DICT_SRC} ${LIBRIME_IMPL_SRC} ${LIBRIME_EXPL_SRC})
target_link_libraries(rime-static
  ${Boost_LIBRARIES} ${Glog_LIBRARY} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${Opencc_LIBRARY}
  ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${RIME_OPTIONAL_DEPS})
set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")

add_library(rime SHARED rime_api.cc)
target_link_libraries(rime rime-static)
set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
set_target_properties(rime PROPERTIES VERSION ${LIBRIME_VERSION} SOVERSION ${LIBRIME_SOVERSION})
if(XCODE_VERSION)
  set_target_properties(rime PROPERTIES INSTALL_NAME_DIR "@rpath")
endif(XCODE_VERSION)

if(NOT BUILD_STATIC)
  install(TARGETS rime DESTINATION ${LIB_INSTALL_DIR})
endif(NOT BUILD_STATIC)
