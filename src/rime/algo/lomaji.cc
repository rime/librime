#include <boost/algorithm/string/predicate.hpp>

namespace rime {

bool BueKamLomaji(const std::string& text) {
  const std::string pi[] = {
    "À", "Á", "Â", "Ă",
    "È", "É", "Ê", "Ĕ",
    "Ì", "Í", "Î", "Ĭ",
    "Ò", "Ó", "Ô", "Ŏ",
    "Ù", "Ú", "Û", "Ŭ",
    "à", "á", "â", "ă",
    "è", "é", "ê", "ĕ",
    "ì", "í", "î", "ĭ",
    "ò", "ó", "ô", "ŏ",
    "ù", "ú", "û", "ŭ",
    "Ā", "ā",
    "Ē", "ē", "Ě", "ě",
    "Ī", "ī", "ı",
    "Ń", "ń", "Ň", "ň",
    "Ō", "ō", "Ő", "ő",
    "Ū", "ū", "Ű", "ű",
    "Ǎ", "ǎ",
    "Ǐ", "ǐ",
    "Ǒ", "ǒ",
    "Ǔ", "ǔ",
    "Ǹ", "ǹ",
    "Ḿ", "ḿ",
    "̀", "̂", "̄", "̋", "̌",
    "̍", "͘",
    "ⁿ"
  };
  size_t len = sizeof(pi)/sizeof(pi[0]);
  for(size_t i=0; i< len; i++) {
    if (boost::algorithm::ends_with(text, pi[i])) {
      return true;
    }
  }
  // p, t, k, h, r, g
  return isalpha(text.back());
}


bool ThauKamLomaji(const std::string& text) {
  const std::string pi[] = {
    "À", "Á", "Â",
    "È", "É", "Ê",
    "Ì", "Í", "Î",
    "Ò", "Ó", "Ô",
    "Ù", "Ú", "Û",
    "à", "á", "â",
    "è", "é", "ê",
    "ì", "í", "î",
    "ò", "ó", "ô",
    "ù", "ú", "û",
    "Ā", "ā",
    "Ē", "ē", "Ě", "ě",
    "Ī", "ī", "ı",
    "Ń", "ń", "Ň", "ň",
    "Ō", "ō", "Ő", "ő",
    "Ū", "ū", "Ű", "ű",
    "Ǎ", "ǎ",
    "Ǐ", "ǐ",
    "Ǒ", "ǒ",
    "Ǔ", "ǔ",
    "Ǹ", "ǹ",
    "Ḿ", "ḿ",
    "̀", "̂", "̄", "̋", "̌",
    "̍", "͘",
    "ⁿ"
  };
  size_t len = sizeof(pi)/sizeof(pi[0]);
  for(size_t i=0; i< len; i++) {
    if (boost::algorithm::starts_with(text, pi[i])) {
      return true;
    }
  }
  // Siann-bó
  return isalpha(text.front());
}

// Sentence
bool KamAiLianJiHu(const std::string& ting_text,
                    const std::string& tsit_text) {
  if (ting_text.empty() || ting_text == " ") {
    return false;
  }

  bool phuann;
  bool ting_kam_lomaji = BueKamLomaji(ting_text);
  bool tsit_kam_lomaji = ThauKamLomaji(tsit_text);

  phuann = ting_kam_lomaji && tsit_kam_lomaji;

  return phuann;
}

}