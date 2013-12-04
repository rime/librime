//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-07-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_ENCODER_H_
#define RIME_ENCODER_H_

#include <set>
#include <string>
#include <vector>
#include <boost/regex.hpp>

namespace rime {

class RawCode : public std::vector<std::string> {
 public:
  std::string ToString() const;
  void FromString(const std::string &code_str);
};

class PhraseCollector {
 public:
  PhraseCollector() {}
  virtual ~PhraseCollector() {}

  virtual void CreateEntry(const std::string& phrase,
                           const std::string& code_str,
                           const std::string& value) = 0;
  // return a list of alternative code for the given word
  virtual bool TranslateWord(const std::string& word,
                             std::vector<std::string>* code) = 0;
};

class Config;

class Encoder {
 public:
  Encoder(PhraseCollector* collector) : collector_(collector) {}
  virtual ~Encoder() {}

  virtual bool LoadSettings(Config* config) {
    return false;
  }

  virtual bool EncodePhrase(const std::string& phrase,
                            const std::string& value) = 0;

  void set_collector(PhraseCollector* collector) { collector_ = collector; }

 protected:
  PhraseCollector* collector_;
};

// Aa : code at index 0 for character at index 0
// Az : code at index -1 for character at index 0
// Za : code at index 0 for character at index -1
struct CodeCoords {
  int char_index;
  int code_index;
};

struct TableEncodingRule {
  int min_word_length;
  int max_word_length;
  std::vector<CodeCoords> coords;
};

// for rule-based phrase encoding
class TableEncoder : public Encoder {
 public:
  TableEncoder(PhraseCollector* collector = NULL);

  bool LoadSettings(Config* config);

  bool Encode(const RawCode& code, std::string* result);
  bool EncodePhrase(const std::string& phrase, const std::string& value);

  bool IsCodeExcluded(const std::string& code);

  bool loaded() const { return loaded_; }
  const std::vector<TableEncodingRule>& encoding_rules() const {
    return encoding_rules_;
  }
  const std::string& tail_anchor() const { return tail_anchor_; }

 protected:
  bool ParseFormula(const std::string& formula, TableEncodingRule* rule);
  int CalculateCodeIndex(const std::string& code, int index, int start);
  bool DfsEncode(const std::string& phrase, const std::string& value,
                 size_t start_pos, RawCode* code, int* limit);

  bool loaded_;
  // settings
  std::vector<TableEncodingRule> encoding_rules_;
  std::vector<boost::regex> exclude_patterns_;
  std::string tail_anchor_;
};

// for syllable-based phrase encoding
class ScriptEncoder : public Encoder {
 public:
  ScriptEncoder(PhraseCollector* collector);

  bool EncodePhrase(const std::string& phrase, const std::string& value);

 private:
  bool DfsEncode(const std::string& phrase, const std::string& value,
                 size_t start_pos, RawCode* code, int* limit);
};

}  // namespace rime

#endif  // RIME_ENCODER_H_
