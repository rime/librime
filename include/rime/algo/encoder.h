//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-07-17 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_ENCODER_H_
#define RIME_ENCODER_H_

#include <boost/regex.hpp>

namespace rime {

class RawCode : public vector<string> {
 public:
  string ToString() const;
  void FromString(const string &code_str);
};

class PhraseCollector {
 public:
  PhraseCollector() = default;
  virtual ~PhraseCollector() = default;

  virtual void CreateEntry(const string& phrase,
                           const string& code_str,
                           const string& value) = 0;
  // return a list of alternative code for the given word
  virtual bool TranslateWord(const string& word,
                             vector<string>* code) = 0;
};

class Config;

class Encoder {
 public:
  Encoder(PhraseCollector* collector) : collector_(collector) {}
  virtual ~Encoder() = default;

  virtual bool LoadSettings(Config* config) {
    return false;
  }

  virtual bool EncodePhrase(const string& phrase,
                            const string& value) = 0;

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
  vector<CodeCoords> coords;
};

// for rule-based phrase encoding
class TableEncoder : public Encoder {
 public:
  TableEncoder(PhraseCollector* collector = NULL);

  bool LoadSettings(Config* config);

  bool Encode(const RawCode& code, string* result);
  bool EncodePhrase(const string& phrase, const string& value);

  bool IsCodeExcluded(const string& code);

  bool loaded() const { return loaded_; }
  const vector<TableEncodingRule>& encoding_rules() const {
    return encoding_rules_;
  }
  const string& tail_anchor() const { return tail_anchor_; }

 protected:
  bool ParseFormula(const string& formula, TableEncodingRule* rule);
  int CalculateCodeIndex(const string& code, int index, int start);
  bool DfsEncode(const string& phrase, const string& value,
                 size_t start_pos, RawCode* code, int* limit);

  bool loaded_;
  // settings
  vector<TableEncodingRule> encoding_rules_;
  vector<boost::regex> exclude_patterns_;
  string tail_anchor_;
  // for optimization
  int max_phrase_length_;
};

// for syllable-based phrase encoding
class ScriptEncoder : public Encoder {
 public:
  ScriptEncoder(PhraseCollector* collector);

  bool EncodePhrase(const string& phrase, const string& value);

 private:
  bool DfsEncode(const string& phrase, const string& value,
                 size_t start_pos, RawCode* code, int* limit);
};

}  // namespace rime

#endif  // RIME_ENCODER_H_
