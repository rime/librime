//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2016-09-08 osfans <waxaca@163.com>
//

#include <rime/candidate.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/ticket.h>
#include <rime/translation.h>
#include <rime/gear/history_translator.h>

namespace rime {

HistoryTranslator::HistoryTranslator(const Ticket& ticket)
    : Translator(ticket),
      tag_("abc"),
      size_(1),
      initial_quality_(1000) {
  if (ticket.name_space == "translator") {
    name_space_ = "history";
  }
  if (!ticket.schema)
    return;
  Config* config = ticket.schema->config();
  config->GetString(name_space_ + "/tag", &tag_);
  config->GetString(name_space_ + "/input", &input_);
  config->GetInt(name_space_ + "/size", &size_);
  config->GetDouble(name_space_ + "/initial_quality",
                    &initial_quality_);
}

an<Translation> HistoryTranslator::Query(const string& input,
                                         const Segment& segment) {
  if (!segment.HasTag(tag_))
    return nullptr;
  if (input_.empty() || input_ != input)
    return nullptr;

  const auto& history(engine_->context()->commit_history());
  if (history.empty())
    return nullptr;
  auto translation = New<FifoTranslation>();
  auto it = history.rbegin();
  int count = 0;
  for (; it != history.rend(); ++it) {
    if (it->type == "thru") continue;
    auto candidate = New<SimpleCandidate>(it->type,
                                          segment.start,
                                          segment.end,
                                          it->text);
    candidate->set_quality(initial_quality_);
    translation->Append(candidate);
    count++;
    if (size_ == count) break;
  }
  return translation;
}

}  // namespace rime
