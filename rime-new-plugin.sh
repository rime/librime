#!/bin/bash

if [[ -z "${1}" ]]; then
  echo "Usage: $(basename "$0") <plugin-name>"
  exit 1
fi

plugin_name="${1/_/-}"
plugin_name="${plugin_name#rime-}"
plugin_dir="plugins/${plugin_name}"
plugin_module="${plugin_name/-/_}"

echo "plugin_name: rime-${plugin_name}"
echo "plugin_dir: ${plugin_dir}"
echo "plugin_module: ${plugin_module}"

mkdir -p "${plugin_dir}"

cat >> "${plugin_dir}/CMakeLists.txt" << //EOF
project(rime-${plugin_name})
cmake_minimum_required(VERSION 3.10)

aux_source_directory(src ${plugin_module}_src)

add_library(rime-${plugin_name}-objs OBJECT \${${plugin_module}_src})
if(BUILD_SHARED_LIBS)
  set_target_properties(rime-${plugin_name}-objs
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
endif()

set(plugin_name rime-${plugin_name} PARENT_SCOPE)
set(plugin_objs \$<TARGET_OBJECTS:rime-${plugin_name}-objs> PARENT_SCOPE)
set(plugin_deps \${rime_library} PARENT_SCOPE)
set(plugin_modules "${plugin_module}" PARENT_SCOPE)
//EOF

mkdir -p "${plugin_dir}/src"

cat >> "${plugin_dir}/src/${plugin_module}_module.cc" << //EOF
#include <rime/component.h>
#include <rime/registry.h>
#include <rime_api.h>

#include "todo_processor.h"

using namespace rime;

static void rime_${plugin_module}_initialize() {
  Registry &r = Registry::instance();
  r.Register("todo_processor", new Component<TodoProcessor>);
}

static void rime_${plugin_module}_finalize() {
}

RIME_REGISTER_MODULE(${plugin_module})
//EOF

cat >> "${plugin_dir}/src/todo_processor.h" << //EOF
#include <rime/common.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/processor.h>

using namespace rime;

class TodoProcessor : public Processor {
 public:
  explicit TodoProcessor(const Ticket& ticket)
    : Processor(ticket) {
    Context* context = engine_->context();
    update_connection_ = context->update_notifier()
      .connect([this](Context* ctx) { OnUpdate(ctx); });
  }

  virtual ~TodoProcessor() {
    update_connection_.disconnect();
  }

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override {
    return kNoop;
  }

 private:
  void OnUpdate(Context* ctx) {}

  connection update_connection_;
};
//EOF
