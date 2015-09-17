// encoding: utf-8
//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
// 2013-10-20 GONG Chen <chen.sst@gmail.com>
//

#include <rime/candidate.h>
#include <rime/segmentation.h>
#include "trivial_translator.h"

namespace sample {

TrivialTranslator::TrivialTranslator(const Ticket& ticket)
    : Translator(ticket) {
  dictionary_["yi"] = "\xe4\xb8\x80";  // 一
  dictionary_["er"] = "\xe4\xba\x8c";  // 二
  dictionary_["san"] = "\xe4\xb8\x89";  // 三
  dictionary_["si"] = "\xe5\x9b\x9b";  // 四
  dictionary_["wu"] = "\xe4\xba\x94";  // 五
  dictionary_["liu"] = "\xe5\x85\xad";  // 六
  dictionary_["qi"] = "\xe4\xb8\x83";  // 七
  dictionary_["ba"] = "\xe5\x85\xab";  // 八
  dictionary_["jiu"] = "\xe4\xb9\x9d";  // 九
  dictionary_["ling"] = "\xe3\x80\x87";  // 〇
  dictionary_["shi"] = "\xe5\x8d\x81";  // 十
  dictionary_["bai"] = "\xe7\x99\xbe";  // 百
  dictionary_["qian"] = "\xe5\x8d\x83";  // 千
  dictionary_["wan"] = "\xe8\x90\xac";  // 萬
}

an<Translation> TrivialTranslator::Query(const string& input,
                                         const Segment& segment) {
  if (!segment.HasTag("abc"))
    return nullptr;
  DLOG(INFO) << "input = '" << input
             << "', [" << segment.start << ", " << segment.end << ")";
  string output = Translate(input);
  if (output.empty()) {
    return nullptr;
  }
  auto candidate = New<SimpleCandidate>(
      "trivial",
      segment.start,
      segment.end,
      output,
      ":-)");
  return New<UniqueTranslation>(candidate);
}

string TrivialTranslator::Translate(const string& input) {
  const size_t kMinPinyinLength = 2;
  const size_t kMaxPinyinLength = 6;
  string result;
  size_t input_len = input.length();
  for (size_t i = 0; i < input_len; ) {
    int translated = 0;
    size_t len = (std::max)(kMaxPinyinLength, input_len - i);
    for ( ; len >= kMinPinyinLength; --len) {
      auto it = dictionary_.find(input.substr(i, len));
      if (it != dictionary_.end()) {
        result += it->second;
        translated = len;
        break;
      }
    }
    if (translated) {
      i += translated;
    }
    else {
      return string();
    }
  }
  return result;
}

}  // namespace sample
