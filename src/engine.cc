// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cctype>
#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/processor.h>
#include <rime/schema.h>

namespace rime {

Engine::Engine() : schema_(new Schema), context_(new Context) {
  EZLOGGERFUNCTRACKER;
  // TODO: load a list of components from schema config
  const std::string processor_name("trivial_processor");
  Processor::Component *pc = Processor::Require(processor_name);
  if (!pc) {
    EZLOGGERPRINT("error creating processor: '%s'", processor_name.c_str());
  }
  else {
    shared_ptr<Processor> p(pc->Create(this));
    processors_.push_back(p);
  }
  // receive notifications on commits
  context_->commit_notifier().connect(
      boost::bind(&Engine::OnCommit, this, _1, _2));
}

Engine::~Engine() {
  EZLOGGERFUNCTRACKER;
  processors_.clear();
  dictionaries_.clear();
}

bool Engine::ProcessKeyEvent(const KeyEvent &key_event) {
  EZLOGGERVAR(key_event);
  BOOST_FOREACH(shared_ptr<Processor> &p, processors_) {
    if (p->ProcessKeyEvent(key_event)) {
      return true;
    }
  }
  return false;
}

void Engine::OnCommit(Context *ctx, const std::string &commit_text) {
  sink_(commit_text);
  ctx->Clear();
}

}  // namespace rime
