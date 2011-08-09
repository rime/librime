aux_source_directory(. LIBRIME_SRC)
aux_source_directory(impl LIBRIME_IMPL_SRC)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(rime-static STATIC ${LIBRIME_SRC} ${LIBRIME_IMPL_SRC})
target_link_libraries(rime-static ${Boost_LIBRARIES} ${YamlCpp_LIBRARY})
set_target_properties(rime-static PROPERTIES OUTPUT_NAME "rime" PREFIX "lib")

add_library(rime SHARED ${LIBRIME_SRC} ${LIBRIME_IMPL_SRC})
target_link_libraries(rime ${Boost_LIBRARIES} ${YamlCpp_LIBRARY})
set_target_properties(rime PROPERTIES VERSION ${LIBRIME_VERSION} SOVERSION ${LIBRIME_SOVERSION})
