// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>

namespace rime {

Engine::Engine() : schema_(NULL), context_(new Context) {
  EZLOGGERFUNCTRACKER;
}

Engine::~Engine() {
  EZLOGGERFUNCTRACKER;
}

}  // namespace rime
