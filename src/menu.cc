
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

int Menu::Prepare(int candidate_count) {
  EZLOGGERFUNCTRACKER;
  int count = candidates_.size();
  if (count >= candidate_count)
    return count;
  while (count < candidate_count && !translations_.empty()) {
    size_t k = 0;
    for (; k + 1 < translations_.size(); ++k) {
      if (translations_[k]->Compare(*translations_[k + 1]) <= 0) {
        break;
      }
    }
    if (k >= translations_.size()) {
      break;
    }

    candidates_.push_back(translations_[k]->Peek());
    translations_[k]->Next();
    ++count;
    if (translations_[k]->exhausted()) {
      EZLOGGERPRINT("Translation #%d exhausted.", k);
      translations_.erase(translations_.begin() + k);
    }
  }
  return candidates_.size();
}

Page* Menu::CreatePage(int page_size, int page_no) {
  int start_pos = page_size * page_no;
  int end_pos = start_pos + page_size;
  if (end_pos > candidates_.size()) {
    if (translations_.empty())
      end_pos = candidates_.size();
    else
      end_pos = Prepare(end_pos);
    if (start_pos >= end_pos)
      return NULL;
  }
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

shared_ptr<Candidate> Menu::GetCandidateAt(int index) {
  if (index >= candidates_.size())
    return shared_ptr<Candidate>();
  return candidates_[index];
}

}  // namespace rime
