//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <boost/algorithm/string.hpp>
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/gear/affix_segmentor.h>

namespace rime {

AffixSegmentor::AffixSegmentor(const Ticket& ticket)
    : Segmentor(ticket), tag_("abc") {
  if (!ticket.schema)
    return;
  if (Config* config = ticket.schema->config()) {
    config->GetString(name_space_ + "/tag", &tag_);
    config->GetString(name_space_ + "/prefix", &prefix_);
    config->GetString(name_space_ + "/suffix", &suffix_);
    config->GetString(name_space_ + "/tips", &tips_);
    config->GetString(name_space_ + "/closing_tips", &closing_tips_);
    if (auto extra_tags = config->GetList(name_space_ + "/extra_tags")) {
      for (size_t i = 0; i < extra_tags->size(); ++i) {
        if (auto value = extra_tags->GetValueAt(i)) {
          extra_tags_.insert(value->str());
        }
      }
    }
  }
}

bool AffixSegmentor::Proceed(Segmentation* segmentation) {
  if (segmentation->empty())
    return true;
  if (!segmentation->back().HasTag(tag_)) {
    if (segmentation->size() >= 2) {
      Segment& previous_segment(*(segmentation->rbegin() + 1));
      if (previous_segment.HasTag("partial") &&
          previous_segment.HasTag(tag_)) {
        // the remaining part of a partial selection should inherit the tag
        segmentation->back().tags.insert(tag_);
        // without adding new tag "abc"
        if (!previous_segment.HasTag("abc")) {
          segmentation->back().tags.erase("abc");
        }
      }
    }
    return true;
  }
  size_t j = segmentation->GetCurrentStartPosition();
  size_t k = segmentation->GetCurrentEndPosition();
  string active_input(segmentation->input().substr(j, k - j));
  if (prefix_.empty() || !boost::starts_with(active_input, prefix_)) {
    return true;
  }
  DLOG(INFO) << "affix_segmentor: " << active_input;
  DLOG(INFO) << "segmentation: " << *segmentation;
  // just prefix
  if (active_input.length() == prefix_.length()) {
    Segment& prefix_segment(segmentation->back());
    prefix_segment.tags.erase(tag_);
    prefix_segment.prompt = tips_;
    prefix_segment.tags.insert(tag_ + "_prefix");
    DLOG(INFO) << "prefix: " << *segmentation;
    // continue this round
    return true;
  }
  // prefix + code
  active_input.erase(0, prefix_.length());
  Segment prefix_segment(j, j + prefix_.length());
  prefix_segment.status = Segment::kGuess;
  prefix_segment.prompt = tips_;
  prefix_segment.tags.insert(tag_ + "_prefix");
  prefix_segment.tags.insert("phony");  // do not commit raw input
  segmentation->pop_back();
  segmentation->Forward();
  segmentation->AddSegment(prefix_segment);
  j += prefix_.length();
  Segment code_segment(j, k);
  code_segment.tags.insert(tag_);
  for (const string& tag : extra_tags_) {
    code_segment.tags.insert(tag);
  }
  segmentation->Forward();
  segmentation->AddSegment(code_segment);
  DLOG(INFO) << "prefix+code: " << *segmentation;
  // has suffix?
  if (!suffix_.empty() && boost::ends_with(active_input, suffix_)) {
    k -= suffix_.length();
    if (k == segmentation->back().start) {
      segmentation->pop_back();  // code is empty
    }
    else {
      segmentation->back().end = k;
    }
    Segment suffix_segment(k, k + suffix_.length());
    suffix_segment.status = Segment::kGuess;
    suffix_segment.prompt = closing_tips_.empty() ? tips_ : closing_tips_;
    suffix_segment.tags.insert(tag_ + "_suffix");
    suffix_segment.tags.insert("phony");  // do not commit raw input
    segmentation->Forward();
    segmentation->AddSegment(suffix_segment);
    DLOG(INFO) << "prefix+suffix: " << *segmentation;
  }
  // exclusive
  return false;
}

}  // namespace rime
