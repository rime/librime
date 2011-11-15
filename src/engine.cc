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
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/processor.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/segmentor.h>
#include <rime/translation.h>
#include <rime/translator.h>

namespace rime {

Engine::Engine() : schema_(new Schema), context_(new Context),
                   auto_commit_(false) {
  EZLOGGERFUNCTRACKER;
  // receive context notifications
  context_->commit_notifier().connect(
      boost::bind(&Engine::OnCommit, this, _1));
  context_->select_notifier().connect(
      boost::bind(&Engine::OnSelect, this, _1));
  context_->update_notifier().connect(
      boost::bind(&Engine::OnContextUpdate, this, _1));
}

Engine::~Engine() {
  EZLOGGERFUNCTRACKER;
  processors_.clear();
  segmentors_.clear();
  translators_.clear();
}

bool Engine::ProcessKeyEvent(const KeyEvent &key_event) {
  EZLOGGERVAR(key_event);
  BOOST_FOREACH(shared_ptr<Processor> &p, processors_) {
    Processor::Result ret = p->ProcessKeyEvent(key_event);
    if (ret == Processor::kRejected) return false;
    if (ret == Processor::kAccepted) return true;
  }
  return false;
}

void Engine::OnContextUpdate(Context *ctx) {
  if (!ctx)
    return;
  const std::string &input(ctx->input());
  EZLOGGERVAR(input);
  Compose(ctx);
}

void Engine::Compose(Context *ctx) {
  if (!ctx)
    return;
  Composition *comp = ctx->composition();
  comp->Reset(ctx->input());
  CalculateSegmentation(comp);
  TranslateSegments(comp);
  ctx->set_composition(comp);
}

void Engine::CalculateSegmentation(Composition *comp) {
  EZLOGGERFUNCTRACKER;
  while (!comp->HasFinished()) {
    int start_pos = comp->GetCurrentStartPosition();
    int end_pos = comp->GetCurrentEndPosition();
    EZLOGGERVAR(start_pos);
    EZLOGGERVAR(end_pos);
    // recognize a segment by calling the segmentors in turn
    BOOST_FOREACH(shared_ptr<Segmentor> &segmentor, segmentors_) {
      if (!segmentor->Proceed(comp))
        break;
    }
    EZLOGGERVAR(*comp);
    // no advancement
    if (start_pos == comp->GetCurrentEndPosition())
      break;
    // move onto the next segment... or 
    // start an empty segment at the end of a confirmed composition.
    if (!comp->HasFinished() ||
        comp->back().status >= Segment::kSelected)
      comp->Forward();
  }

}

void Engine::TranslateSegments(Composition *comp) {
  EZLOGGERFUNCTRACKER;
  BOOST_FOREACH(Segment &segment, *comp) {
    if (segment.status >= Segment::kSelected)
      continue;
    int len = segment.end - segment.start;
    if (len <= 0) continue;
    const std::string input(comp->input().substr(segment.start, len));
    EZLOGGERPRINT("Translating segment '%s'", input.c_str());
    shared_ptr<Menu> menu(new Menu);
    BOOST_FOREACH(shared_ptr<Translator> translator, translators_) {
      shared_ptr<Translation> translation(translator->Query(input, segment));
      if (!translation)
        continue;
      if (translation->exhausted()) {
        EZLOGGERPRINT("Oops, got a futile translation.");
        continue;
      }
      menu->AddTranslation(translation);
    }
    segment.status = Segment::kGuess;
    segment.menu = menu;
    segment.selected_index = 0;
  }
}

void Engine::OnCommit(Context *ctx) {
  const std::string commit_text = ctx->GetCommitText();
  EZLOGGERVAR(commit_text);
  sink_(commit_text);
}

void Engine::OnSelect(Context *ctx) {
  Segment &seg(ctx->composition()->back());
  shared_ptr<Candidate> cand(seg.GetSelectedCandidate());
  if (!cand) return;
  if (cand->end() < seg.end) {
    // having selected a partially matched candidate, split it into 2 segments
    seg.end = cand->end();
  }
  if (seg.end == ctx->input().length()) {
    // composition has finished
    seg.status = Segment::kConfirmed;
    // strategy one: commit directly;
    // strategy two: continue composing with another empty segment.
    if (auto_commit_)
      ctx->Commit();
    else
      ctx->composition()->Forward();
  }
  else {
    if (seg.end >= ctx->cursor()) {
      // finished converting current segment
      // move cursor to the end of input
      // TODO: not implemented
      //ctx->set_cursor(ctx->input().length());
    }
    ctx->composition()->Forward();
    Compose(ctx);
  }
}

void Engine::set_schema(Schema *schema) {
  schema_.reset(schema);
  InitializeComponents();
}

void Engine::InitializeComponents() {
  if (!schema_)
    return;
  Config *config = schema_->config();
  // create processors
  shared_ptr<ConfigList> processor_list(config->GetList("engine/processors"));
  if (processor_list) {
    size_t n = processor_list->size();
    for (size_t i = 0; i < n; ++i) {
      shared_ptr<ConfigValue> klass = As<ConfigValue>(processor_list->GetAt(i));
      if (klass) continue;
      Processor::Component *c = Processor::Require(klass->str());
      if (!c) {
        EZLOGGERPRINT("error creating processor: '%s'", klass->str().c_str());
      }
      else {
        shared_ptr<Processor> p(c->Create(this));
        processors_.push_back(p);
      }
    }
  }
  // create segmentors
  shared_ptr<ConfigList> segmentor_list(config->GetList("engine/segmentors"));
  if (segmentor_list) {
    size_t n = segmentor_list->size();
    for (size_t i = 0; i < n; ++i) {
      shared_ptr<ConfigValue> klass = As<ConfigValue>(segmentor_list->GetAt(i));
      if (klass) continue;
      Segmentor::Component *c = Segmentor::Require(klass->str());
      if (!c) {
        EZLOGGERPRINT("error creating segmentor: '%s'", klass->str().c_str());
      }
      else {
        shared_ptr<Segmentor> s(c->Create(this));
        segmentors_.push_back(s);
      }
    }
  }
  // create translators
  shared_ptr<ConfigList> translator_list(config->GetList("engine/translators"));
  if (translator_list) {
    size_t n = translator_list->size();
    for (size_t i = 0; i < n; ++i) {
      shared_ptr<ConfigValue> klass = As<ConfigValue>(translator_list->GetAt(i));
      if (klass) continue;
      Translator::Component *c = Translator::Require(klass->str());
      if (!c) {
        EZLOGGERPRINT("error creating translator: '%s'", klass->str().c_str());
      }
      else {
        shared_ptr<Translator> d(c->Create(this));
        translators_.push_back(d);
      }
    }
  }
}

}  // namespace rime
