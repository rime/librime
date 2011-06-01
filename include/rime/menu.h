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
#include <rime/common.h>

namespace rime {

class Candidate;
class Translation;

struct Page {
  int page_size;
  int page_no;
  bool is_last;
  std::vector<shared_ptr<Candidate> > candidates;
};

class Menu {
 public:
  Menu() {}
  ~Menu() {}

  void AddTranslation(shared_ptr<Translation> translation);
  void Prepare(int candidate_count);
  Page* CreatePage(int page_size, int page_no);
  
 private:
  std::vector<shared_ptr<Translation> > translations_;
  std::vector<shared_ptr<Candidate> > candidates_;
};

}  // namespace rime

#endif  // RIME_MENU_H_
