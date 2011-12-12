// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2011-12-12 GONG Chen <chen.sst@gmail.com>
//
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/scoped_array.hpp>
#include <opencc.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/engine.h>
#include <rime/service.h>
#include <rime/impl/simplifier.h>
#include <stdint.h>
#include <utf8.h>

static const char *quote_left = "\xe3\x80\x94";  //"\xef\xbc\x88";
static const char *quote_right = "\xe3\x80\x95";  //"\xef\xbc\x89";

namespace rime {

class Opencc {
 public:
  Opencc();
  ~Opencc();
  bool Convert(const shared_ptr<Candidate> &original,
               CandidateList *result);
 private:
  opencc_t od_;
};

Opencc::Opencc() {
  boost::filesystem::path shared_data_dir(Service::instance().deployer().shared_data_dir);
  boost::filesystem::path config_path = shared_data_dir / "opencc" / "zht2zhs.ini";
  od_ = opencc_open(config_path.string().c_str());
  if (od_ == (opencc_t) -1) {
    EZLOGGERPRINT("Error opening opencc.");
  }
}

Opencc::~Opencc() {
  if (od_ != (opencc_t) -1) {
    opencc_close(od_);
  }
}

bool Opencc::Convert(const shared_ptr<Candidate> &original,
                     CandidateList *result) {
  if (od_ == (opencc_t) -1)
    return false;
  const std::string &text(original->text());
  boost::scoped_array<uint32_t> inbuf(new uint32_t[text.length() + 1]);
  uint32_t *end = utf8::unchecked::utf8to32(text.c_str(), text.c_str() + text.length(), inbuf.get());
  *end = L'\0';
  size_t inlen = end - inbuf.get();
  uint32_t *inptr = inbuf.get();
  size_t outlen = inlen * 5;
  boost::scoped_array<uint32_t> outbuf(new uint32_t[outlen + 1]);
  uint32_t *outptr = outbuf.get();
  bool single_char = false;
  if (inlen == 1) {
    single_char = true;
    opencc_set_conversion_mode(od_, OPENCC_CONVERSION_LIST_CANDIDATES);
  }
  else {
    opencc_set_conversion_mode(od_, OPENCC_CONVERSION_FAST);
  }
  size_t converted = opencc_convert(od_, &inptr, &inlen, &outptr, &outlen);
  if (!converted) {
    EZLOGGERPRINT("Error simplifying '%s'.", text.c_str());
    return false;
  }
  *outptr = L'\0';
  boost::scoped_array<char> out_utf8(new char[(outptr - outbuf.get()) * 6 + 1]);
  char *utf8_end = utf8::unchecked::utf32to8(outbuf.get(), outptr, out_utf8.get());
  *utf8_end = '\0';
  std::string simplified(out_utf8.get());
  if (simplified == text) {
    return false;
  }
  if (single_char) {
    std::vector<std::string> forms;
    boost::split(forms, simplified, boost::is_any_of(" "));
    for (size_t i = 0; i < forms.size(); ++i) {
      if (forms[i] == original->text()) {
        result->push_back(original);
      }
      else {
        result->push_back(shared_ptr<Candidate>(new ShadowCandidate(
            original,
            "zh_simplified",
            forms[i],
            quote_left + original->text() + quote_right)));
      }
    }
  }
  else {
    result->push_back(shared_ptr<Candidate>(new ShadowCandidate(
        original,
        "zh_simplified",
        simplified)));
  }
  return true;
}


// Simplifier

Simplifier::Simplifier(Engine *engine) : Filter(engine), opencc_(new Opencc) {
}

Simplifier::~Simplifier() {
}

bool Simplifier::Proceed(CandidateList *recruited,
                         CandidateList *candidates) {
  if (!opencc_ ||
      !engine_->get_option("simplification"))
    return true;
  if (!candidates || candidates->empty())
    return false;
  CandidateList result;
  for (CandidateList::iterator it = candidates->begin();
       it != candidates->end(); ++it) {
    if (!opencc_->Convert(*it, &result))
      result.push_back(*it);
  }
  candidates->swap(result);
  return true;
}

}  // namespace rime
