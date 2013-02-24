#include <boost/scoped_array.hpp>
#include <stdint.h>
#include <utf8.h>
#include <rime/common.h>
#include <rime/gear/opencc.h>

namespace rime {

Opencc::Opencc(const std::string &config_path) {
  LOG(INFO) << "initilizing opencc: " << config_path;
  od_ = opencc_open(config_path.c_str());
  if (od_ == (opencc_t) -1) {
    LOG(ERROR) << "Error opening opencc.";
  }
}

Opencc::~Opencc() {
  if (od_ != (opencc_t) -1) {
    opencc_close(od_);
  }
}

bool Opencc::ConvertText(const std::string &text, std::string *simplified, bool *is_single_char) {
  if (od_ == (opencc_t) -1)
    return false;
  boost::scoped_array<uint32_t> inbuf(new uint32_t[text.length() + 1]);
  uint32_t *end = utf8::unchecked::utf8to32(text.c_str(), text.c_str() + text.length(), inbuf.get());
  *end = L'\0';
  size_t inlen = end - inbuf.get();
  uint32_t *inptr = inbuf.get();
  size_t outlen = inlen * 5;
  boost::scoped_array<uint32_t> outbuf(new uint32_t[outlen + 1]);
  uint32_t *outptr = outbuf.get();
  if (inlen == 1) {
    *is_single_char = true;
    opencc_set_conversion_mode(od_, OPENCC_CONVERSION_LIST_CANDIDATES);
  }
  else {
    *is_single_char = false;
    opencc_set_conversion_mode(od_, OPENCC_CONVERSION_FAST);
  }
  size_t converted = opencc_convert(od_, &inptr, &inlen, &outptr, &outlen);
  if (!converted) {
    LOG(ERROR) << "Error simplifying '" << text << "'.";
    return false;
  }
  *outptr = L'\0';
  boost::scoped_array<char> out_utf8(new char[(outptr - outbuf.get()) * 6 + 1]);
  char *utf8_end = utf8::unchecked::utf32to8(outbuf.get(), outptr, out_utf8.get());
  *utf8_end = '\0';
  *simplified = out_utf8.get();
  return true;
}

}  // namespace rime
