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

}  // namespace rime
