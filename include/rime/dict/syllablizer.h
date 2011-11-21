// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-7-12 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_SYLLABLIZER_H_
#define RIME_SYLLABLIZER_H_

#include <map>

namespace rime {

class Prism;

typedef int SyllableId;

enum SpellingType { kNormalSpelling, kAbbreviation, kCompletion };

struct SpellingProperties {
  SpellingType type;
  double credibility;
  std::string tips;
  SpellingProperties() : type(kNormalSpelling), credibility(1.0) {}
};

typedef std::map<SyllableId, SpellingProperties> SpellingMap;
typedef std::map<size_t, SpellingType> VertexMap;
typedef std::map<size_t, SpellingMap> EndVertexMap;
typedef std::map<size_t, EndVertexMap> EdgeMap;

struct SyllableGraph {
  size_t input_length;
  size_t interpreted_length;
  VertexMap vertices;
  EdgeMap edges;
  SyllableGraph() : input_length(0), interpreted_length(0) {}
};

class Syllablizer {
 public:
  Syllablizer() : enable_completion_(false) {}
  explicit Syllablizer(const std::string &delimiters, bool enable_completion = false)
      : delimiters_(delimiters), enable_completion_(enable_completion) {}
  
  int BuildSyllableGraph(const std::string &input, Prism &prism, SyllableGraph *graph);

 protected:
  std::string delimiters_;
  bool enable_completion_;
};

}  // namespace rime

#endif  // RIME_SYLLABLIZER_H_
