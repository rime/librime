//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-01-04 GONG Chen <chen.sst@gmail.com>
//

#include <rime_api.h>
#include <rime/common.h>
#include <rime/registry.h>

#include "trivial_translator.h"

using namespace rime;

static void rime_sample_initialize() {
  LOG(INFO) << "registering components from module 'sample'.";
  Registry& r = Registry::instance();
  r.Register("trivial_translator", new Component<sample::TrivialTranslator>);
}

static void rime_sample_finalize() {
}

RIME_REGISTER_MODULE(sample)
