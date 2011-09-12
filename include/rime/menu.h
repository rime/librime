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
  Menu() {}
  ~Menu() {}

  void AddTranslation(shared_ptr<Translation> translation);
  int Prepare(int candidate_count);
  Page* CreatePage(int page_size, int page_no);
  shared_ptr<Candidate> GetCandidateAt(int index);

  // CAVEAT: returns the number of candidates currently obtained,
  // rather than the total number of available candidates.
  int candidate_count() const { return candidates_.size(); }

 private:
  std::vector<shared_ptr<Translation> > translations_;
  CandidateList candidates_;
};

}  // namespace rime

#endif  // RIME_MENU_H_
