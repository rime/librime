//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <stdint.h>
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/translation.h>
#include <rime/gear/simplifier.h>
#include <opencc/Config.hpp> // Place OpenCC #includes here to avoid VS2015 compilation errors
#include <opencc/Converter.hpp>
#include <opencc/Conversion.hpp>
#include <opencc/ConversionChain.hpp>
#include <opencc/Dict.hpp>
#include <opencc/DictEntry.hpp>

static const char* quote_left = "\xe3\x80\x94";  //"\xef\xbc\x88";
static const char* quote_right = "\xe3\x80\x95";  //"\xef\xbc\x89";

namespace rime {

class Opencc {
 public:
  Opencc(const string& config_path) {
    LOG(INFO) << "initilizing opencc: " << config_path;
    opencc::Config config;
    converter_ = config.NewFromFile(config_path);
    const list<opencc::ConversionPtr> conversions =
      converter_->GetConversionChain()->GetConversions();
    dict_ = conversions.front()->GetDict();
  }

  bool ConvertSingleCharacter(const string& text,
                              vector<string>* forms) {
    opencc::Optional<const opencc::DictEntry*> item = dict_->Match(text);
    if (item.IsNull()) {
      // Match not found
      return false;
    } else {
      const opencc::DictEntry* entry = item.Get();
      for (const char* value : entry->Values()) {
        forms->push_back(value);
      }
      return true;
    }
  }

  bool ConvertText(const string& text,
                   string* simplified) {
    *simplified = converter_->Convert(text);
    return true;
  }

 private:
   opencc::ConverterPtr converter_;
   opencc::DictPtr dict_;
};

// Simplifier

Simplifier::Simplifier(const Ticket& ticket) : Filter(ticket),
                                               TagMatching(ticket) {
  if (name_space_ == "filter") {
    name_space_ = "simplifier";
  }
  if (Config* config = engine_->schema()->config()) {
    string tips;
    if (config->GetString(name_space_ + "/tips", &tips) ||
        config->GetString(name_space_ + "/tip", &tips)) {
      tips_level_ = (tips == "all") ? kTipsAll :
                    (tips == "char") ? kTipsChar : kTipsNone;
    }
    config->GetString(name_space_ + "/option_name", &option_name_);
    config->GetString(name_space_ + "/opencc_config", &opencc_config_);
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
  if (opencc_config_.empty()) {
    opencc_config_ = "t2s.json";  // default opencc config file
  }
}

void Simplifier::Initialize() {
  using namespace boost::filesystem;
  initialized_ = true;  // no retry
  path opencc_config_path = opencc_config_;
  if (opencc_config_path.extension().string() == ".ini") {
    LOG(ERROR) << "please upgrade opencc_config to an opencc 1.0 config file.";
    return;
  }
  if (opencc_config_path.is_relative()) {
    path user_config_path = Service::instance().deployer().user_data_dir;
    path shared_config_path = Service::instance().deployer().shared_data_dir;
    (user_config_path /= "opencc") /= opencc_config_path;
    (shared_config_path /= "opencc") /= opencc_config_path;
    if (exists(user_config_path)) {
      opencc_config_path = user_config_path;
    }
    else if (exists(shared_config_path)) {
      opencc_config_path = shared_config_path;
    }
  }
  try {
    opencc_.reset(new Opencc(opencc_config_path.string()));
  }
  catch (opencc::Exception& e) {
    LOG(ERROR) << "Error initializing opencc: " << e.what();
  }
}

class SimplifiedTranslation : public PrefetchTranslation {
 public:
  SimplifiedTranslation(an<Translation> translation,
                        Simplifier* simplifier)
      : PrefetchTranslation(translation), simplifier_(simplifier) {
  }

 protected:
  virtual bool Replenish();

  Simplifier* simplifier_;
};


bool SimplifiedTranslation::Replenish() {
  auto next = translation_->Peek();
  translation_->Next();
  if (!simplifier_->Convert(next, &cache_)) {
    cache_.push_back(next);
  }
  return !cache_.empty();
}

an<Translation> Simplifier::Apply(an<Translation> translation,
                                          CandidateList* candidates) {
  if (!engine_->context()->get_option(option_name_)) {  // off
    return translation;
  }
  if (!initialized_) {
    Initialize();
  }
  if (!opencc_) {
    return translation;
  }
  return New<SimplifiedTranslation>(translation, this);
}

bool Simplifier::Convert(const an<Candidate>& original,
                         CandidateQueue* result) {
  if (excluded_types_.find(original->type()) != excluded_types_.end()) {
    return false;
  }
  size_t length = utf8::unchecked::distance(original->text().c_str(),
                                            original->text().c_str()
                                            + original->text().length());
  bool success;
  vector<string> forms;
  success = opencc_->ConvertSingleCharacter(original->text(), &forms);
  if (success && forms.size() > 0) {
    for (size_t i = 0; i < forms.size(); ++i) {
      if (forms[i] == original->text()) {
        result->push_back(original);
      }
      else {
        string tips;
        if (tips_level_ >= kTipsChar) {
          tips = quote_left + original->text() + quote_right;
        }
        result->push_back(
            New<ShadowCandidate>(
                original,
                "simplified",
                forms[i],
                tips));
      }
    }
  } else if (length > 1) {
    string simplified;
    success = opencc_->ConvertText(original->text(), &simplified);
    if (!success || simplified == original->text()) {
      return false;
    }
    string tips;
    if (tips_level_ == kTipsAll) {
      tips = quote_left + original->text() + quote_right;
    }
    result->push_back(
        New<ShadowCandidate>(
            original,
            "simplified",
            simplified,
            tips));
  } else {
    return false;
  }
  return true;
}

}  // namespace rime
