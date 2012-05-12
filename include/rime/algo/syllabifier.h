// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-7-12 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_SYLLABIFIER_H_
#define RIME_SYLLABIFIER_H_

#include <map>
#include <string>
#include "spelling.h"

namespace rime {

class Prism;

typedef int SyllableId;

typedef std::map<SyllableId, SpellingProperties> SpellingMap;
typedef std::map<size_t, SpellingType> VertexMap;
typedef std::map<size_t, SpellingMap> EndVertexMap;
typedef std::map<size_t, EndVertexMap> EdgeMap;

typedef std::vector<const SpellingProperties*> SpellingPropertiesList;
typedef std::map<SyllableId, SpellingPropertiesList> SpellingIndex;
typedef std::map<size_t, SpellingIndex> SpellingIndices;

struct SyllableGraph {
  size_t input_length;
  size_t interpreted_length;
  VertexMap vertices;
  EdgeMap edges;
  SpellingIndices indices;
  SyllableGraph() : input_length(0), interpreted_length(0) {}
};

class Syllabifier {
 public:
  Syllabifier() : enable_completion_(false) {}
  explicit Syllabifier(const std::string &delimiters, bool enable_completion = false)
      : delimiters_(delimiters), enable_completion_(enable_completion) {}
  
  int BuildSyllableGraph(const std::string &input, Prism &prism, SyllableGraph *graph);

 protected:
  void CheckOverlappedSpellings(SyllableGraph *graph, size_t start, size_t end);
  void Transpose(SyllableGraph* graph);
  
  std::string delimiters_;
  bool enable_completion_;
};

}  // namespace rime

#endif  // RIME_SYLLABIFIER_H_
