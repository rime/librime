set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)


set(RIME_CONSOLE_SRC "rime_console.cc")
add_executable(rime_console ${RIME_CONSOLE_SRC})
target_link_libraries(rime_console rime-static)
add_dependencies(rime_console rime-static)


set(PRIME_SRC "prime.cc")
add_executable(prime ${PRIME_SRC})
target_link_libraries(prime rime)
add_dependencies(prime rime)


file(COPY ${PROJECT_SOURCE_DIR}/data/rime_console.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/default.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/data/samples/luna_pinyin.dict.yaml 
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
