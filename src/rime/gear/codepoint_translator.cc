//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2016-09-08 osfans <waxaca@163.com>
//

#include <rime/candidate.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/gear/codepoint_translator.h>
#include <rime/gear/translator_commons.h>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace rime {

CodepointTranslator::CodepointTranslator(const Ticket& ticket)
    : Translator(ticket), tag_("charset") {
  if (!ticket.schema)
    return;
  Config* config = ticket.schema->config();
  config->GetString(name_space_ + "/tag", &tag_);
}

void CodepointTranslator::Initialize() {
  initialized_ = true;  // no retry
  if (!engine_)
    return;
  Ticket ticket(engine_, name_space_);
  Config* config = engine_->schema()->config();
  if (!config)
    return;
  config->GetString(name_space_ + "/prefix", &prefix_);
  config->GetString(name_space_ + "/suffix", &suffix_);
  config->GetString(name_space_ + "/tips", &tips_);
  config->GetString(name_space_ + "/charset", &charset_);
  converters_["utf"] = [](const string& code) {
      string s;
      uint32_t i = 0;
      sscanf(code.c_str(), "%x", &i);
      if (i > 0) {
        s = boost::locale::conv::utf_to_utf<char>(&i, &i+1);
      }
      return s;
  };
  converters_["xml"] = [](const string& code) {
      string s;
      uint32_t i = 0;
      sscanf(code.c_str(), "%u", &i);
      if (i > 0) {
        s = boost::locale::conv::utf_to_utf<char>(&i, &i+1);
      }
      return s;
  };
  converters_["quwei"] = [](const string& code) {
      string s;
      uint16_t i = 0;
      sscanf(code.c_str(), "%hu", &i);
      if (i >= 1601 && i <= 9494) {
        i = (i/100 + 0xa0)<<8 | (i%100 + 0xa0);
        i = ntohs(i);
        s = boost::locale::conv::to_utf<char>((const char*)&i, "gb2312");
      }
      return s;
  };
  converters_[""] = converters_["codepoint"] = converters_["utf"];  // aliases
  converters_["dec"] = converters_["xml"];
}

static string conv_to_utf(const string& input, const string& charset) {
    string s;
    uint32_t codepoint = 0;
    string code = input;
    size_t n = code.length();
    size_t padding = 8 - (n % 8);
    if (padding < 8) code.append(padding, '0');
    sscanf(code.c_str(), "%x", &codepoint);
    if (codepoint > 0) {
      codepoint = ntohl(codepoint);
      const char* c = (const char*)&codepoint;
      s += boost::locale::conv::to_utf<char>(c, c + 4, charset);
    }
    return s;
}

an<Translation> CodepointTranslator::Query(const string& input,
                                      const Segment& segment) {
  if (!segment.HasTag(tag_))
    return nullptr;
  if (!initialized_)
    Initialize();
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";

  size_t start = 0;
  if (!prefix_.empty() && boost::starts_with(input, prefix_)) {
    start = prefix_.length();
  }
  string code = input.substr(start);
  if (!suffix_.empty() && boost::ends_with(code, suffix_)) {
    code.resize(code.length() - suffix_.length());
  }

  if (start > 0) {
    // usually translators do not modify the segment directly;
    // prompt text is best set by a processor or a segmentor.
    const_cast<Segment*>(&segment)->prompt = tips_;
  }

  if (code.length() == 0) return nullptr;
  try {
    string converted;
    if (converters_.find(charset_) != converters_.end()) {
      converted = converters_[charset_](code);
    } else {
      converted = conv_to_utf(code, charset_);
    }
    if (converted.empty()) return nullptr;
    auto candidate = New<SimpleCandidate>("raw",
                                          segment.start,
                                          segment.end,
                                          converted);
    return New<UniqueTranslation>(candidate);
  } catch (...) {
    return nullptr;
  }
}

}  // namespace rime
