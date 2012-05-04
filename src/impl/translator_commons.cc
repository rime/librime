// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2012 RIME Developers
// License: GPLv3
//
// 2012-04-22 GONG Chen <chen.sst@gmail.com>
//
#include <rime/config.h>
#include <rime/impl/translator_commons.h>

namespace rime {

// Patterns

bool Patterns::Load(ConfigListPtr patterns) {
  clear();
  if (!patterns) return false;
  for (ConfigList::Iterator it = patterns->begin(); it != patterns->end(); ++it) {
    ConfigValuePtr value = As<ConfigValue>(*it);
    if (!value) continue;
    push_back(boost::regex(value->str()));
  }
  return true;
}

// Sentence

void Sentence::Extend(const DictEntry& entry, size_t end_pos) {
  const double kEpsilon = 1e-200;
  const double kPenalty = 1e-8;
  entry_.code.insert(entry_.code.end(),
                     entry.code.begin(), entry.code.end());
  entry_.text.append(entry.text);
  entry_.weight *= (std::max)(entry.weight, kEpsilon) * kPenalty;
  components_.push_back(entry);
  set_end(end_pos);
  EZDBGONLYLOGGERPRINT("%d) %s : %g", end_pos,
                       entry_.text.c_str(), entry_.weight);
}

void Sentence::Offset(size_t offset) {
  set_start(start() + offset);
  set_end(end() + offset);
}

// TableTranslation

TableTranslation::TableTranslation(const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(true);
}

TableTranslation::TableTranslation(const DictEntryIterator& iter,
                                   const std::string &input,
                                   size_t start, size_t end,
                                   const std::string &preedit,
                                   Projection *comment_formatter)
    : iter_(iter), input_(input), start_(start), end_(end),
      preedit_(preedit), comment_formatter_(comment_formatter) {
  set_exhausted(iter_.exhausted());
}

bool TableTranslation::Next() {
  if (exhausted())
    return false;
  iter_.Next();
  set_exhausted(iter_.exhausted());
  return true;
}

shared_ptr<Candidate> TableTranslation::Peek() {
  if (exhausted())
    return shared_ptr<Candidate>();
  const shared_ptr<DictEntry> &e(iter_.Peek());
  std::string comment(e->comment);
  if (comment_formatter_) {
    comment_formatter_->Apply(&comment);
  }
  shared_ptr<Candidate> cand(new SimpleCandidate(
      "zh",
      start_,
      end_,
      e->text,
      comment,
      preedit_));
  return cand;
}

}  // namespace rime
