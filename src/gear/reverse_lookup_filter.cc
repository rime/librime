//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-11-05 GONG Chen <chen.sst@gmail.com>
//
#include <boost/foreach.hpp>
#include <rime/candidate.h>
#include <rime/engine.h>
#include <rime/schema.h>
#include <rime/dict/reverse_lookup_dictionary.h>
#include <rime/gear/reverse_lookup_filter.h>
#include <rime/gear/translator_commons.h>

namespace rime {

ReverseLookupFilter::ReverseLookupFilter(const Ticket& ticket)
    : Filter(ticket), initialized_(false), overwrite_comment_(false) {
  if (ticket.name_space == "filter") {
    name_space_ = "reverse_lookup";
  }
}

void ReverseLookupFilter::Initialize() {
  initialized_ = true;
  if (!engine_) return;
  Ticket ticket(engine_, name_space_);
  if (ReverseLookupDictionary::Component* c =
      ReverseLookupDictionary::Require("reverse_lookup_dictionary")) {
    rev_dict_.reset(c->Create(ticket));
    if (rev_dict_ && !rev_dict_->Load()) {
      rev_dict_.reset();
    }
  }
  if (Config* config = engine_->schema()->config()) {
    config->GetBool(name_space_ + "/overwrite_comment", &overwrite_comment_);
    comment_formatter_.Load(
        config->GetList(name_space_ + "/comment_format"));
  }
}

void ReverseLookupFilter::Apply(CandidateList *recruited,
                                CandidateList *candidates) {
  if (!initialized_)
    Initialize();
  if (!rev_dict_)
    return;
  BOOST_FOREACH(shared_ptr<Candidate>& c, *candidates) {
    if (!overwrite_comment_ && !c->comment().empty())
      continue;
    shared_ptr<Phrase> phrase = As<Phrase>(Candidate::GetGenuineCandidate(c));
    if (!phrase)
      continue;
    std::string codes;
    if (rev_dict_->ReverseLookup(phrase->text(), &codes)) {
      comment_formatter_.Apply(&codes);
      if (!codes.empty()) {
        phrase->set_comment(codes);
      }
    }
  }
}

}  // namespace rime
