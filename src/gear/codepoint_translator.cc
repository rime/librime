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

  int n = code.length();
  if (n == 0) return nullptr;
  string s = "";
  try {
    if (charset_.compare("") == 0 || charset_.compare("utf") == 0
        || charset_.compare("codepoint") == 0) {
      uint32_t i = 0;
      sscanf(code.c_str(), "%x", &i);
      if (i == 0) return nullptr;
      s = boost::locale::conv::utf_to_utf<char>(&i, &i+1);
    } else if (charset_.compare("dec") == 0 || charset_.compare("xml") == 0) {
      uint32_t i = 0;
      sscanf(code.c_str(), "%u", &i);
      if (i == 0) return nullptr;
      s = boost::locale::conv::utf_to_utf<char>(&i, &i+1);
    } else if (charset_.compare("quwei") == 0) {
      uint16_t i = 0;
      sscanf(code.c_str(), "%hu", &i);
      if (i < 1601 || i > 9494) return nullptr;
      i = (i/100 + 0xa0)<<8 | (i%100 + 0xa0);
      i = ntohs(i);
      s = boost::locale::conv::to_utf<char>((const char*)&i, "gb2312");
    } else {
      uint32_t i = 0;
      if (n < 8) code.append(8 - n, '0');
      sscanf(code.c_str(), "%x", &i);
      if (i == 0) return nullptr;
      i = ntohl(i);
      const char* c = (const char*)&i;
      s = boost::locale::conv::to_utf<char>(c, c + 4, charset_);
    }
  }
  catch (...) {
    LOG(ERROR) << "Error conv charset.";
    return nullptr;
  }
  auto candidate = New<SimpleCandidate>("raw",
                                        segment.start,
                                        segment.end,
                                        s);
  return New<UniqueTranslation>(candidate);
}

}  // namespace rime
