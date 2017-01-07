//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-05-29 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <iterator>
#include <rime/filter.h>
#include <rime/menu.h>
#include <rime/translation.h>

namespace rime {

Menu::Menu()
    : merged_(new MergedTranslation(candidates_)),
      result_(merged_) {
}

void Menu::AddTranslation(an<Translation> translation) {
  *merged_ += translation;
  DLOG(INFO) << merged_->size() << " translations added.";
}

void Menu::AddFilter(Filter* filter) {
  result_ = filter->Apply(result_, &candidates_);
}

size_t Menu::Prepare(size_t requested) {
  DLOG(INFO) << "preparing " << requested << " candidates.";
  while (candidates_.size() < requested && !result_->exhausted()) {
    if (auto cand = result_->Peek()) {
      candidates_.push_back(cand);
    }
    result_->Next();
  }
  return candidates_.size();
}

Page* Menu::CreatePage(size_t page_size, size_t page_no) {
  size_t start_pos = page_size * page_no;
  size_t end_pos = start_pos + page_size;
  if (end_pos > candidates_.size()) {
    if (result_->exhausted())
      end_pos = candidates_.size();
    else
      end_pos = Prepare(end_pos);
    if (start_pos >= end_pos)
      return NULL;
    end_pos = (std::min)(start_pos + page_size, end_pos);
  }
  Page* page = new Page;
  if (!page)
    return NULL;
  page->page_size = page_size;
  page->page_no = page_no;
  page->is_last_page = result_->exhausted() && (end_pos == candidates_.size());
  std::copy(candidates_.begin() + start_pos,
            candidates_.begin() + end_pos,
            std::back_inserter(page->candidates));
  return page;
}

an<Candidate> Menu::GetCandidateAt(size_t index) {
  if (index >= candidates_.size() &&
      index >= Prepare(index + 1)) {
    return nullptr;
  }
  return candidates_[index];
}

bool Menu::empty() const {
  return candidates_.empty() && result_->exhausted();
}

}  // namespace rime
