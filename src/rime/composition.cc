//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-06-19 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/candidate.h>
#include <rime/composition.h>
#include <rime/menu.h>

namespace rime {

bool Composition::HasFinishedComposition() const {
  if (empty())
    return false;
  size_t k = size() - 1;
  if (k > 0 && at(k).start == at(k).end)
    --k;
  return at(k).status >= Segment::kSelected;
}

Preedit Composition::GetPreedit(const string& full_input, size_t caret_pos,
                                const string& caret) const {
  Preedit preedit;
  if (empty()) {
    return preedit;
  }
  preedit.caret_pos = string::npos;
  size_t start = 0;
  size_t end = 0;
  for (size_t i = 0; i < size(); ++i) {
    start = end;
    if (caret_pos == start) {
      preedit.caret_pos = preedit.text.length();
    }
    auto cand = at(i).GetSelectedCandidate();
    if (i < size() - 1) {  // converted
      if (cand) {
        end = cand->end();
        preedit.text += cand->text();
      }
      else {  // raw input
        end = at(i).end;
        if (!at(i).HasTag("phony")) {
          preedit.text += input_.substr(start, end - start);
        }
      }
    }
    else {  // highlighted
      preedit.sel_start = preedit.text.length();
      preedit.sel_end = string::npos;
      if (cand && !cand->preedit().empty()) {
        end = cand->end();
        auto caret_placeholder = cand->preedit().find('\t');
        if (caret_placeholder != string::npos) {
          preedit.text += cand->preedit().substr(0, caret_placeholder);
          // the part after caret is considered prompt string,
          // show it only when the caret is at the end of input.
          if (caret_pos == end && end == full_input.length()) {
            preedit.sel_end = preedit.sel_start + caret_placeholder;
            preedit.caret_pos = preedit.sel_end;
            preedit.text += cand->preedit().substr(caret_placeholder + 1);
          }
        } else {
          preedit.text += cand->preedit();
        }
      }
      else {
        end = at(i).end;
        preedit.text += input_.substr(start, end - start);
      }
      if (preedit.sel_end == string::npos) {
        preedit.sel_end = preedit.text.length();
      }
    }
  }
  if (end < input_.length()) {
    preedit.text += input_.substr(end);
    end = input_.length();
  }
  if (preedit.caret_pos == string::npos) {
    preedit.caret_pos = preedit.text.length();
  }
  if (end < full_input.length()) {
    preedit.text += full_input.substr(end);
  }
  // insert soft cursor and prompt string.
  auto prompt = caret + GetPrompt();
  if (!prompt.empty()) {
    preedit.text.insert(preedit.caret_pos, prompt);
    if (preedit.caret_pos < preedit.sel_end) {
      preedit.sel_start += prompt.length();
      preedit.sel_end += prompt.length();
      preedit.caret_pos = preedit.sel_start;
    }
  }
  return preedit;
}

string Composition::GetPrompt() const {
  return empty() ? string() : back().prompt;
}

string Composition::GetCommitText() const {
  string result;
  size_t end = 0;
  for (const Segment& seg : *this) {
    if (auto cand = seg.GetSelectedCandidate()) {
      end = cand->end();
      result += cand->text();
    }
    else {
      end = seg.end;
      if (!seg.HasTag("phony")) {
        result += input_.substr(seg.start, seg.end - seg.start);
      }
    }
  }
  if (input_.length() > end) {
    result += input_.substr(end);
  }
  return result;
}

string Composition::GetScriptText() const {
  string result;
  size_t start = 0;
  size_t end = 0;
  for (const Segment& seg : *this) {
    auto cand = seg.GetSelectedCandidate();
    start = end;
    end = cand ? cand->end() : seg.end;
    if (cand && !cand->preedit().empty())
      result += boost::erase_first_copy(cand->preedit(), "\t");
    else
      result += input_.substr(start, end - start);
  }
  if (input_.length() > end) {
    result += input_.substr(end);
  }
  return result;
}

string Composition::GetDebugText() const {
  string result;
  int i = 0;
  for (const Segment& seg : *this) {
    if (i++ > 0)
      result += "|";
    if (!seg.tags.empty()) {
      result += "{";
      int j = 0;
      for (const string& tag : seg.tags) {
        if (j++ > 0)
          result += ",";
        result += tag;
      }
      result += "}";
    }
    result += input_.substr(seg.start, seg.end - seg.start);
    if (auto cand = seg.GetSelectedCandidate()) {
      result += "=>";
      result += cand->text();
    }
  }
  return result;
}

}  // namespace rime
