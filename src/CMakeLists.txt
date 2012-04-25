set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. LIBRIME_SRC)
aux_source_directory(algo LIBRIME_ALGO_SRC)
aux_source_directory(dict LIBRIME_DICT_SRC)
aux_source_directory(impl LIBRIME_IMPL_SRC)
aux_source_directory(expl LIBRIME_EXPL_SRC)

if (BUILD_STATIC)

add_library(rime STATIC ${LIBRIME_SRC} ${LIBRIME_ALGO_SRC} ${LIBRIME_DICT_SRC} ${LIBRIME_IMPL_SRC} ${LIBRIME_EXPL_SRC})
target_link_libraries(rime
  ${Boost_LIBRARIES} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${Opencc_LIBRARY}
  ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(rime PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")

else (BUILD_STATIC)

add_library(rime SHARED ${LIBRIME_SRC} ${LIBRIME_ALGO_SRC} ${LIBRIME_DICT_SRC} ${LIBRIME_IMPL_SRC} ${LIBRIME_EXPL_SRC})
target_link_libraries(rime
  ${Boost_LIBRARIES} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${Opencc_LIBRARY}
  ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
set_target_properties(rime PROPERTIES VERSION ${LIBRIME_VERSION} SOVERSION ${LIBRIME_SOVERSION})
install(TARGETS rime DESTINATION ${LIB_INSTALL_DIR})

endif (BUILD_STATIC)
