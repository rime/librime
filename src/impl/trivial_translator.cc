// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-02 Wensong He <snowhws@gmail.com>
//

#include <rime/candidate.h>
#include <rime/segmentation.h>
#include <rime/impl/trivial_translator.h>

namespace rime {

TrivialTranslator::TrivialTranslator(Engine *engine)
    : Translator(engine) {
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

Translation* TrivialTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (!segment.HasTag("abc"))
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);
  std::string output(Translate(input));
  if (output.empty())
    return NULL;
  shared_ptr<Candidate> candidate(
      new SimpleCandidate("abc", segment.start, segment.end, output, ":-)"));

  Translation *translation = new UniqueTranslation(candidate);
  return translation;
}

const std::string TrivialTranslator::Translate(const std::string &input) {
  const size_t kMinPinyinLength = 2;
  const size_t kMaxPinyinLength = 6;
  std::string result;
  size_t input_len = input.length();
  for (size_t i = 0; i < input_len; ) {
    int translated = 0;
    size_t len = (std::max)(kMaxPinyinLength, input_len - i);
    for ( ; len >= kMinPinyinLength; --len) {
      TrivialDictionary::const_iterator it =
          dictionary_.find(input.substr(i, len));
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
      return std::string();
    }
  }
  return result;
}

}  // namespace rime
