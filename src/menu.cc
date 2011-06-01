// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//
#include <iterator>
#include <rime/menu.h>
#include <rime/translation.h>

namespace rime {

void Menu::AddTranslation(shared_ptr<Translation> translation) {
  translations_.push_back(translation);
}

void Menu::Prepare(int candidate_count) {
  int count = candidates_.size();
  if (count >= candidate_count)
    return;
  while (count < candidate_count && !translations_.empty()) {
    std::vector<shared_ptr<Translation> >::iterator winner(translations_.begin());
    while (winner != translations_.end()) {
      std::vector<shared_ptr<Translation> >::iterator next(winner + 1);
      if (next == translations_.end() || (*winner)->Compare(**next) <= 0)
        break;
      ++winner;
    }
    if (winner == translations_.end())
      break;
    candidates_.push_back((*winner)->Next());
    ++count;
    if ((*winner)->exhausted()) {
      translations_.erase(winner);
    }
  }
}

Page* Menu::CreatePage(int page_size, int page_no) {
  int start_pos = page_size * page_no;
  if (start_pos >= candidates_.size())
    return NULL;
  int end_pos = std::min(start_pos + page_size,
                         static_cast<int>(candidates_.size()));
  Page *page = new Page;
  if (!page)
    return NULL;
  page->page_size = page_size;
  page->page_no = page_no;
  page->is_last = (translations_.empty()) && (end_pos == candidates_.size());
  std::copy(candidates_.begin() + start_pos, candidates_.begin() + end_pos,
            std::back_inserter(page->candidates));
  return page;
}

}  // namespace rime
