set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

aux_source_directory(. LIBRIME_SRC)
aux_source_directory(impl LIBRIME_IMPL_SRC)
aux_source_directory(api LIBRIME_API_SRC)

add_library(rime-static STATIC ${LIBRIME_SRC} ${LIBRIME_IMPL_SRC} ${LIBRIME_API_SRC})
target_link_libraries(rime-static ${Boost_LIBRARIES} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${ZLIB_LIBRARY})
set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")

#add_library(rime SHARED ${LIBRIME_SRC} ${LIBRIME_IMPL_SRC} ${LIBRIME_API_SRC})
#target_link_libraries(rime ${Boost_LIBRARIES} ${YamlCpp_LIBRARY} ${KyotoCabinet_LIBRARY} ${ZLIB_LIBRARY})
#set_target_properties(rime PROPERTIES DEFINE_SYMBOL "RIME_EXPORTS")
#set_target_properties(rime PROPERTIES VERSION ${LIBRIME_VERSION} SOVERSION ${LIBRIME_SOVERSION})
