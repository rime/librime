//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-7-12 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_SYLLABIFIER_H_
#define RIME_SYLLABIFIER_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>
#include "spelling.h"

namespace rime {

struct Syllabification {
  std::vector<size_t> vertices;

  // move by syllable by returning a value different from caret_pos
  size_t PreviousStop(size_t caret_pos) const;
  size_t NextStop(size_t caret_pos) const;

  bool empty() const {
    return vertices.empty();
  }
  size_t start() const {
    return empty() ? 0 : vertices.front();
  }
  size_t end() const {
    return empty() ? 0 : vertices.back();
  }
};

class Prism;

using SyllableId = int32_t;

using SpellingMap = std::map<SyllableId, SpellingProperties>;
using VertexMap = std::map<size_t, SpellingType>;
using EndVertexMap = std::map<size_t, SpellingMap>;
using EdgeMap = std::map<size_t, EndVertexMap>;

using SpellingPropertiesList = std::vector<const SpellingProperties*>;
using SpellingIndex = std::map<SyllableId, SpellingPropertiesList>;
using SpellingIndices = std::map<size_t, SpellingIndex>;

struct SyllableGraph {
  size_t input_length = 0;
  size_t interpreted_length = 0;
  VertexMap vertices;
  EdgeMap edges;
  SpellingIndices indices;
};

class Syllabifier {
 public:
  Syllabifier() = default;
  explicit Syllabifier(const std::string &delimiters,
                       bool enable_completion = false,
                       bool strict_spelling = false)
      : delimiters_(delimiters),
        enable_completion_(enable_completion),
        strict_spelling_(strict_spelling) {
  }

  int BuildSyllableGraph(const std::string &input,
                         Prism &prism,
                         SyllableGraph *graph);

 protected:
  void CheckOverlappedSpellings(SyllableGraph *graph,
                                size_t start, size_t end);
  void Transpose(SyllableGraph* graph);

  std::string delimiters_;
  bool enable_completion_ = false;
  bool strict_spelling_ = false;
};

}  // namespace rime

#endif  // RIME_SYLLABIFIER_H_
