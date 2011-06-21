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
#include "trivial_translator.h"

namespace rime {

TrivialTranslator::TrivialTranslator(Engine *engine)
    : Translator(engine) {
  dictionary_["dao"] = "到";
  dictionary_["jin"] = "今";
  dictionary_["jiu"] = "就";
  dictionary_["luo"] = "洛";
  dictionary_["ming"] = "鳴";
  dictionary_["nan"] = "難";
  dictionary_["sheng"] = "生";
  dictionary_["shi"] = "世";
  dictionary_["shuo"] = "說";
  dictionary_["wen"] = "問";
  dictionary_["wo"] = "我";
  dictionary_["yang"] = "陽";
  dictionary_["yao"] = "要";
  dictionary_["yuan"] = "冤";
  dictionary_["zai"] = "在";
  dictionary_["zhan"] = "斬";
  dictionary_["zhi"] = "只";
}

Translation* TrivialTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (segment.tags.find("abc") == segment.tags.end())
    return NULL;
  EZLOGGERPRINT("input = '%s', [%d, %d)",
                input.c_str(), segment.start, segment.end);
  std::string output(Translate(input));
  shared_ptr<Candidate> candidate(
      new Candidate("abc", output, ":-)", segment.start, segment.end, 0));
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
    size_t len = std::max(kMaxPinyinLength, input_len - i);
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
      result += input[i];
      ++i;
    }
  }
  return result;
}

}  // namespace rime
