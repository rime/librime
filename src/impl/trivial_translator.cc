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
  dictionary_["yi"] = "一";
  dictionary_["er"] = "二";
  dictionary_["san"] = "三";
  dictionary_["si"] = "四";
  dictionary_["wu"] = "五";
  dictionary_["liu"] = "六";
  dictionary_["qi"] = "七";
  dictionary_["ba"] = "八";
  dictionary_["jiu"] = "九";
  dictionary_["ling"] = "〇";
  dictionary_["shi"] = "十";
  dictionary_["bai"] = "百";
  dictionary_["qian"] = "千";
  dictionary_["wan"] = "萬";
}

Translation* TrivialTranslator::Query(const std::string &input,
                                      const Segment &segment) {
  if (!segment.HasTag("abc"))
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
