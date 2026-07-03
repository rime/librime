//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <stdint.h>
#include <utf8.h>
#include <utility>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/translation.h>
#include <rime/gear/opencc.h>
#include <rime/gear/simplifier.h>

static const char* quote_left = "\xe3\x80\x94";   //"\xef\xbc\x88";
static const char* quote_right = "\xe3\x80\x95";  //"\xef\xbc\x89";

namespace rime {

// Simplifier

Simplifier::Simplifier(const Ticket& ticket, an<Opencc> opencc)
    : Filter(ticket), TagMatching(ticket), opencc_(opencc) {
  if (name_space_ == "filter") {
    name_space_ = "simplifier";
  }
  Schema* schema = engine_ ? engine_->schema() : nullptr;
  if (Config* config = schema ? schema->config() : nullptr) {
    string tips;
    if (config->GetString(name_space_ + "/tips", &tips) ||
        config->GetString(name_space_ + "/tip", &tips)) {
      tips_level_ = (tips == "all")    ? kTipsAll
                    : (tips == "char") ? kTipsChar
                                       : kTipsNone;
    }
    config->GetBool(name_space_ + "/show_in_comment", &show_in_comment_);
    config->GetBool(name_space_ + "/inherit_comment", &inherit_comment_);
    comment_formatter_.Load(config->GetList(name_space_ + "/comment_format"));
    config->GetBool(name_space_ + "/random", &random_);
    config->GetString(name_space_ + "/option_name", &option_name_);
    if (auto types = config->GetList(name_space_ + "/excluded_types")) {
      for (auto it = types->begin(); it != types->end(); ++it) {
        if (auto value = As<ConfigValue>(*it)) {
          excluded_types_.insert(value->str());
        }
      }
    }
  }
  if (option_name_.empty()) {
    option_name_ = "simplification";  // default switcher option
  }
  if (random_) {
    srand((unsigned)time(NULL));
  }
}

class SimplifiedTranslation : public PrefetchTranslation {
 public:
  SimplifiedTranslation(an<Translation> translation, Simplifier* simplifier)
      : PrefetchTranslation(translation), simplifier_(simplifier) {}

 protected:
  virtual bool Replenish();

  Simplifier* simplifier_;
};

bool SimplifiedTranslation::Replenish() {
  auto next = translation_->Peek();
  translation_->Next();
  if (next && !simplifier_->Convert(next, &cache_)) {
    cache_.push_back(next);
  }
  return !cache_.empty();
}

an<Translation> Simplifier::Apply(an<Translation> translation,
                                  CandidateList* candidates) {
  if (!engine_->context()->get_option(option_name_)) {  // off
    return translation;
  }
  if (!opencc_) {
    return translation;
  }
  return New<SimplifiedTranslation>(translation, this);
}

void Simplifier::PushBack(const an<Candidate>& original,
                          CandidateQueue* result,
                          const string& simplified) {
  string tips;
  string text;
  size_t length = utf8::unchecked::distance(
      original->text().c_str(),
      original->text().c_str() + original->text().length());
  bool show_tips =
      (tips_level_ == kTipsChar && length == 1) || tips_level_ == kTipsAll;
  if (show_in_comment_) {
    text = original->text();
    if (show_tips) {
      tips = simplified;
      comment_formatter_.Apply(&tips);
    }
  } else {
    text = simplified;
    if (show_tips) {
      tips = original->text();
      bool modified = comment_formatter_.Apply(&tips);
      if (!modified) {
        tips = quote_left + original->text() + quote_right;
      }
    }
  }
  result->push_back(New<ShadowCandidate>(original, "simplified", text, tips,
                                         inherit_comment_));
}

bool Simplifier::Convert(const an<Candidate>& original,
                         CandidateQueue* result) {
  if (excluded_types_.find(original->type()) != excluded_types_.end()) {
    return false;
  }
  bool success = false;
  if (random_) {
    string simplified;
    success = opencc_->RandomConvertText(original->text(), &simplified);
    if (success) {
      PushBack(original, result, simplified);
    }
  } else {  //! random_
    vector<string> forms;
    success = opencc_->ConvertWord(original->text(), &forms);
    if (success) {
      for (size_t i = 0; i < forms.size(); ++i) {
        if (forms[i] == original->text()) {
          result->push_back(original);
        } else {
          PushBack(original, result, forms[i]);
        }
      }
    } else {
      string simplified;
      success = opencc_->ConvertText(original->text(), &simplified);
      if (success) {
        PushBack(original, result, simplified);
      }
    }
  }
  return success;
}

SimplifierComponent::SimplifierComponent() {}

Simplifier* SimplifierComponent::Create(const Ticket& ticket) {
  string name_space = ticket.name_space;
  if (name_space == "filter") {
    name_space = "simplifier";
  }
  string opencc_config;
  an<Opencc> opencc;
  if (Config* config = ticket.engine->schema()->config()) {
    config->GetString(name_space + "/opencc_config", &opencc_config);
  }
  if (opencc_config.empty()) {
    opencc_config = "t2s.json";  // default opencc config file
  }
  opencc = opencc_map_[opencc_config].lock();
  if (opencc) {
    return new Simplifier(ticket, opencc);
  }
  path opencc_config_path = path(opencc_config);
  if (opencc_config_path.extension().u8string() == ".ini") {
    LOG(ERROR) << "please upgrade opencc_config to an opencc 1.0 config file.";
    return nullptr;
  }
  if (opencc_config_path.is_relative()) {
    path user_config_path = Service::instance().deployer().user_data_dir;
    path shared_config_path = Service::instance().deployer().shared_data_dir;
    (user_config_path /= "opencc") /= opencc_config_path;
    (shared_config_path /= "opencc") /= opencc_config_path;
    if (exists(user_config_path)) {
      opencc_config_path = user_config_path;
    } else if (exists(shared_config_path)) {
      opencc_config_path = shared_config_path;
    }
  }
  opencc = New<Opencc>(opencc_config_path);
  // 以原始配置中的文件路径作为 key，避免重复查找文件
  opencc_map_[opencc_config] = opencc;
  return new Simplifier(ticket, opencc);
}

}  // namespace rime
