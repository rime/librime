//
// Copyleft RIME Developers
// License: GPLv3
//
// 2013-10-30 GONG Chen <chen.sst@gmail.com>
//
#include <rime/common.h>
#include <rime/schema.h>
#include <rime/segmentation.h>
#include <rime/gear/affix_segmentor.h>

namespace rime {

AffixSegmentor::AffixSegmentor(const Ticket& ticket)
    : Segmentor(ticket), tag_("abc") {
  // read schema settings
  if (!ticket.schema) return;
  if (Config *config = ticket.schema->config()) {
    config->GetString(name_space_ + "/tag", &tag_);
    config->GetString(name_space_ + "/prefix", &prefix_);
    config->GetString(name_space_ + "/suffix", &suffix_);
    config->GetString(name_space_ + "/tips", &tips_);
  }
}

bool AffixSegmentor::Proceed(Segmentation *segmentation) {
  const std::string &input(segmentation->input());
  DLOG(INFO) << "affix_segmentor: " << input;
  // TODO
  // continue this round
  return true;
}

}  // namespace rime
