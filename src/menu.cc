
// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <iterator>
#include <rime/menu.h>
#include <rime/translation.h>

namespace rime {

void Menu::AddTranslation(shared_ptr<Translation> translation) {
  translations_.push_back(translation);
  EZDBGONLYLOGGERVAR(translations_.size());
}

size_t Menu::Prepare(size_t candidate_count) {
  EZDBGONLYLOGGERFUNCTRACKER;
  size_t count = candidates_.size();
  if (count >= candidate_count)
    return count;
  while (count < candidate_count && !translations_.empty()) {
    size_t k = 0;
    for (; k < translations_.size(); ++k) {
      shared_ptr<Translation> next;
      if (k + 1 < translations_.size())
        next = translations_[k + 1];
      if (translations_[k]->Compare(next, candidates_) <= 0) {
        break;
      }
    }
    if (k >= translations_.size()) {
      EZDBGONLYLOGGERPRINT("Failed to select a winner translation.");
      break;
    }
    if (translations_[k]->exhausted()) {
      EZLOGGERPRINT("Warning: selected translation #%d has been exhausted!", k);
      translations_.erase(translations_.begin() + k);
      continue;
    }
    CandidateList next_candidates;
    next_candidates.push_back(translations_[k]->Peek());
    if (filter_) {
      filter_(&candidates_, &next_candidates);
    }
    if (next_candidates.empty()) {
      EZLOGGERPRINT("Filter returns empty candidate list.");
    }
    else {
      EZDBGONLYLOGGERPRINT("Recruiting %d candidates.", next_candidates.size());
      std::copy(next_candidates.begin(), next_candidates.end(),
                std::back_inserter(candidates_));
    }
    count = candidates_.size();
    translations_[k]->Next();
    if (translations_[k]->exhausted()) {
      EZDBGONLYLOGGERPRINT("Translation #%d has been exhausted.", k);
      translations_.erase(translations_.begin() + k);
    }
  }
  return count;
}

Page* Menu::CreatePage(size_t page_size, size_t page_no) {
  size_t start_pos = page_size * page_no;
  size_t end_pos = start_pos + page_size;
  if (end_pos > candidates_.size()) {
    if (translations_.empty())
      end_pos = candidates_.size();
    else
      end_pos = Prepare(end_pos);
    if (start_pos >= end_pos)
      return NULL;
    end_pos = (std::min)(start_pos + page_size, end_pos);
  }
  Page *page = new Page;
  if (!page)
    return NULL;
  page->page_size = page_size;
  page->page_no = page_no;
  page->is_last_page = (translations_.empty()) && (end_pos == candidates_.size());
  std::copy(candidates_.begin() + start_pos,
            candidates_.begin() + end_pos,
            std::back_inserter(page->candidates));
  return page;
}

shared_ptr<Candidate> Menu::GetCandidateAt(size_t index) {
  if (index >= candidates_.size() &&
      index >= Prepare(index + 1)) {
    return shared_ptr<Candidate>();
  }
  return candidates_[index];
}

}  // namespace rime
