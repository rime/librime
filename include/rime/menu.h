// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MENU_H_
#define RIME_MENU_H_

#include <vector>
#include <boost/function.hpp>
#include <rime/candidate.h>
#include <rime/common.h>

namespace rime {

class Translation;

struct Page {
  int page_size;
  int page_no;
  bool is_last_page;
  CandidateList candidates;
};

class Menu {
 public:
  typedef boost::function<void (CandidateList *recruited,
                                CandidateList *candidates)>
  CandidateFilter;

  Menu() {}
  Menu(const CandidateFilter &filter) : filter_(filter) {}
  ~Menu() {}

  void AddTranslation(shared_ptr<Translation> translation);
  size_t Prepare(size_t candidate_count);
  Page* CreatePage(size_t page_size, size_t page_no);
  shared_ptr<Candidate> GetCandidateAt(size_t index);

  // CAVEAT: returns the number of candidates currently obtained,
  // rather than the total number of available candidates.
  size_t candidate_count() const { return candidates_.size(); }

 private:
  std::vector<shared_ptr<Translation> > translations_;
  CandidateList candidates_;
  CandidateFilter filter_;
};

}  // namespace rime

#endif  // RIME_MENU_H_
