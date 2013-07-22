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

/*
# sample encoder configuration (from cangjie5.dict.yaml)
encoder:
  exclude_patterns:
  - '^x.*$'
  - '^z.*$'
  rules:
  - length_equal: 2
    formula: "AaAzBaBbBz"
  - length_equal: 3
    formula: "AaAzBaBzCz"
  - length_in_range: [4, 10]
    formula: "AaBzCaYzZz"
  tail_anchor: "'"
*/

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

class Config;

class TableEncoder {
 public:
  TableEncoder();

  void LoadSettings(Config* config);
  bool LoadFromFile(const std::string& filename);

  bool Encode(const std::vector<std::string>& code, std::string* result);

  bool IsCodeExcluded(const std::string& code);

  bool loaded() const { return loaded_; }
  const std::vector<TableEncodingRule>& encoding_rules() const {
    return encoding_rules_;
  }
  const std::string& tail_anchor() const { return tail_anchor_; }

 protected:
  bool ParseFormula(const std::string& formula, TableEncodingRule* rule);
  // index: 0-based virtual index of encoding characters in `code`.
  //        counting from the end of `code` if `index` is negative.
  //        tail anchors do not count as encoding characters.
  // start: when `index` is negative, the first appearance of a tail anchor
  //        beyond `start` is used to locate the encoding character at index -1.
  // returns string index in `code` for the character at virtual `index`.
  // may return a negative number if `index` does not exist in `code`.
  int CalculateCodeIndex(const std::string& code, int index, int start);

  bool loaded_;
  // settings
  std::vector<TableEncodingRule> encoding_rules_;
  std::vector<boost::regex> exclude_patterns_;
  std::string tail_anchor_;
};

}  // namespace rime

#endif  // RIME_ENCODER_H_
