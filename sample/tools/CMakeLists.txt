set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(SAMPLE_CONSOLE_SRC "sample_console.cc")
add_executable(sample_console ${SAMPLE_CONSOLE_SRC})
target_link_libraries(sample_console rime-sample ${Rime_LIBRARIES})
add_dependencies(sample_console rime-sample)

file(COPY ${PROJECT_SOURCE_DIR}/tools/default.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
file(COPY ${PROJECT_SOURCE_DIR}/tools/sample.schema.yaml
     DESTINATION ${EXECUTABLE_OUTPUT_PATH})
