//
// Copyleft RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <opencc/opencc.h>
#include <stdint.h>
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/service.h>
#include <rime/gear/simplifier.h>

static const char* quote_left = "\xe3\x80\x94";  //"\xef\xbc\x88";
static const char* quote_right = "\xe3\x80\x95";  //"\xef\xbc\x89";

namespace rime {

class Opencc {
 public:
  Opencc(const std::string& config_path) {
    LOG(INFO) << "initilizing opencc: " << config_path;
    converter_ = unique_ptr<opencc::SimpleConverter>(
      new opencc::SimpleConverter(config_path));
  }
  bool ConvertText(const std::string& text,
                   std::string* simplified,
                   bool* is_single_char) {
    //FIXME is_single_char not set
    *simplified = converter_->Convert(text);
    return true;
  }

 private:
  unique_ptr<opencc::SimpleConverter> converter_;
};

// Simplifier

Simplifier::Simplifier(const Ticket& ticket) : Filter(ticket),
                                               TagMatching(ticket) {
  if (name_space_ == "filter") {
    name_space_ = "simplifier";
  }
  if (Config* config = engine_->schema()->config()) {
    std::string tips;
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
  opencc_.reset(new Opencc(opencc_config_path.string()));
}

void Simplifier::Apply(CandidateList* recruited,
                       CandidateList* candidates) {
  if (!engine_->context()->get_option(option_name_))  // off
    return;
  if (!initialized_)
    Initialize();
  if (!opencc_ || !candidates || candidates->empty())
    return;
  CandidateList result;
  for (auto it = candidates->begin(); it != candidates->end(); ++it) {
    if (!Convert(*it, &result))
      result.push_back(*it);
  }
  candidates->swap(result);
}

bool Simplifier::Convert(const shared_ptr<Candidate>& original,
                         CandidateList* result) {
  if (excluded_types_.find(original->type()) != excluded_types_.end()) {
    return false;
  }
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
        std::string tips;
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
  }
  else {
    std::string tips;
    if (tips_level_ == kTipsAll) {
      tips = quote_left + original->text() + quote_right;
    }
    result->push_back(
        New<ShadowCandidate>(
            original,
            "simplified",
            simplified,
            tips));
  }
  return true;
}

}  // namespace rime
