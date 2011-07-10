aux_source_directory(. RIME_TOOLS_SRC)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(rime_console ${RIME_TOOLS_SRC})
target_link_libraries(rime_console
                      rime 
                      ${Boost_LIBRARIES} 
                      ${CMAKE_THREAD_LIBS_INIT} 
                      ${YamlCpp_LIBRARY})
add_dependencies(rime_console rime)

file(COPY ${PROJECT_SOURCE_DIR}/data/rime_console.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/samples/luna_pinyin.dict.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
