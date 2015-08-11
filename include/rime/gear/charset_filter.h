//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#ifndef RIME_CHARSET_FILTER_H_
#define RIME_CHARSET_FILTER_H_

#include <rime/filter.h>
#include <rime/translation.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class CharsetFilterTranslation : public Translation {
 public:
  CharsetFilterTranslation(a<Translation> translation);
  virtual bool Next();
  virtual a<Candidate> Peek();

 protected:
  bool LocateNextCandidate();

  a<Translation> translation_;
};

struct DictEntry;

class CharsetFilter : public Filter, TagMatching {
 public:
  explicit CharsetFilter(const Ticket& ticket);

  virtual a<Translation> Apply(a<Translation> translation,
                                        CandidateList* candidates);

  virtual bool AppliesToSegment(Segment* segment) {
    return TagsMatch(segment);
  }

  // return true to accept, false to reject the tested item
  static bool FilterText(const string& text);
  static bool FilterDictEntry(a<DictEntry> entry);
};

}  // namespace rime

#endif  // RIME_CHARSET_FILTER_H_
