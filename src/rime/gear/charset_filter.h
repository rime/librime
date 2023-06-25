//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
// 2019-12-26  Chen Gong <chen.sst@gmail.com>  reduced to basic implementation
//
#ifndef RIME_CHARSET_FILTER_H_
#define RIME_CHARSET_FILTER_H_

#include <rime_api.h>
#include <rime/filter.h>
#include <rime/translation.h>
#include <rime/gear/filter_commons.h>

namespace rime {

class CharsetFilterTranslation : public Translation {
 public:
  explicit CharsetFilterTranslation(an<Translation> translation);
  virtual bool Next();
  virtual an<Candidate> Peek();

 protected:
  virtual bool FilterCandidate(an<Candidate> cand);

  bool LocateNextCandidate();

  an<Translation> translation_;
};

struct DictEntry;

class CharsetFilter : public Filter, TagMatching {
 public:
  explicit CharsetFilter(const Ticket& ticket);

  virtual an<Translation> Apply(an<Translation> translation,
                                CandidateList* candidates);

  virtual bool AppliesToSegment(Segment* segment) { return TagsMatch(segment); }

  // return true to accept, false to reject the tested item
  static bool FilterText(const string& text);
  static bool FilterDictEntry(an<DictEntry> entry);
};

}  // namespace rime

#endif  // RIME_CHARSET_FILTER_H_
