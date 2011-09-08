// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-05-22 GONG Chen <chen.sst@gmail.com>
//

#include <rime/candidate.h>

namespace rime {

Candidate::Candidate()
    : type_(), text_(), prompt_(),
      start_(0), end_(0), order_(0) {
}

Candidate::Candidate(const std::string type,
                     const std::string text,
                     const std::string prompt,
                     int start,
                     int end,
                     int order)
    : type_(type), text_(text), prompt_(prompt),
      start_(start), end_(end), order_(order) {
}

Candidate::~Candidate() {
}

}  // namespace rime
