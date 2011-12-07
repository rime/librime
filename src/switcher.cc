// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-07 GONG Chen <chen.sst@gmail.com>
//
#include <string>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/switcher.h>
#include <rime/key_event.h>
#include <rime/schema.h>

namespace rime {

Switcher::Switcher() : schema_(new Schema),
                       context_(new Context),
                       active_(false) {
  EZLOGGERFUNCTRACKER;
  // receive context notifications
  context_->select_notifier().connect(
      boost::bind(&Switcher::OnSelect, this, _1));
}

Switcher::~Switcher() {
  EZLOGGERFUNCTRACKER;
}

bool Switcher::ProcessKeyEvent(const KeyEvent &key_event) {
  // TODO: activate switcher menu
  return false;
}

Schema* Switcher::CreateSchema() {
  Config *config = schema_->config();
  if (!config) return NULL;
  ConfigListPtr schema_list = config->GetList("schema_list");
  if (!schema_list) return NULL;
  ConfigMapPtr item = As<ConfigMap>(schema_list->GetAt(0));
  if (!item) return NULL;
  ConfigValuePtr schema_id = item->GetValue("schema");
  if (!schema_id) return NULL;
  return new Schema(schema_id->str());
}

void Switcher::OnSelect(Context *ctx) {
  Segment &seg(ctx->composition()->back());
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (!cand) return;
  // TODO:
}

}  // namespace rime
