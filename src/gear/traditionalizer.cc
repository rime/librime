#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_array.hpp>
#include <stdint.h>
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/gear/opencc.h>
#include <rime/gear/traditionalizer.h>

static const char *quote_left = "\xe3\x80\x94";  //"\xef\xbc\x88";
static const char *quote_right = "\xe3\x80\x95";  //"\xef\xbc\x89";

namespace rime {

// Traditionalizer

Traditionalizer::Traditionalizer(Engine *engine) : Filter(engine),
                                                   initialized_(false),
                                                   tip_level_(kTipNone) {
  Config *config = engine->schema()->config();
  if (config) {
    std::string tip;
    if (config->GetString("traditionalizer/tip", &tip)) {
      tip_level_ =
          (tip == "all") ? kTipAll :
          (tip == "char") ? kTipChar : kTipNone;
    }
    config->GetString("traditionalizer/option_name", &option_name_);
    config->GetString("traditionalizer/opencc_config", &opencc_config_);
  }
  if (option_name_.empty()) {
    option_name_ = "traditionalizing";  // default switcher option
  }
  if (opencc_config_.empty()) {
    opencc_config_ = "zhs2zht.ini";  // default opencc config file
  }
}

Traditionalizer::~Traditionalizer() {
}

void Traditionalizer::Initialize() {
  initialized_ = true;  // no retry
  boost::filesystem::path opencc_config_path = opencc_config_;
  if (opencc_config_path.is_relative()) {
    boost::filesystem::path user_config_path(Service::instance().deployer().user_data_dir);
    boost::filesystem::path shared_config_path(Service::instance().deployer().shared_data_dir);
    (user_config_path /= "opencc") /= opencc_config_path;
    (shared_config_path /= "opencc") /= opencc_config_path;
    if (boost::filesystem::exists(user_config_path)) {
      opencc_config_path = user_config_path;
    }
    else if (boost::filesystem::exists(shared_config_path)) {
      opencc_config_path = shared_config_path;
    }
  }
  opencc_.reset(new Opencc(opencc_config_path.string()));
}

bool Traditionalizer::Proceed(CandidateList *recruited,
                              CandidateList *candidates) {
  if (!engine_->context()->get_option(option_name_))  // off
    return true;
  if (!initialized_) Initialize();
  if (!opencc_ || !candidates || candidates->empty())
    return true;
  CandidateList result;
  for (CandidateList::iterator it = candidates->begin();
       it != candidates->end(); ++it) {
    if (!Convert(*it, &result))
      result.push_back(*it);
  }
  candidates->swap(result);
  return true;
}

bool Traditionalizer::Convert(const shared_ptr<Candidate> &original,
                              CandidateList *result) {
  std::string simplified;
  bool is_single_char = false;
  if (!opencc_->ConvertText(original->text(), &simplified, &is_single_char) ||
      simplified == original->text()) {
    return false;
  }
  if (is_single_char) {
    std::vector<std::string> forms;
    boost::split(forms, simplified, boost::is_any_of(" "));
    for (size_t i = 0; i < forms.size(); ++i) {
      if (forms[i] == original->text()) {
        result->push_back(original);
      }
      else {
        std::string tip;
        if (tip_level_ >= kTipChar) {
          tip = quote_left + original->text() + quote_right;
        }
        result->push_back(
            boost::make_shared<ShadowCandidate>(
                original,
                "zh_traditionalized",
                forms[i],
                tip));
      }
    }
  }
  else {
    std::string tip;
    if (tip_level_ == kTipAll) {
      tip = quote_left + original->text() + quote_right;
    }
    result->push_back(
        boost::make_shared<ShadowCandidate>(
            original,
            "zh_traditionalized",
            simplified,
            tip));
  }
  return true;
}

}  // namespace rime
