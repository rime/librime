//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2013-07-02 GONG Chen <chen.sst@gmail.com>
//
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/gear/shape.h>

namespace rime {

void ShapeFormatter::Format(string* text) {
  if (!engine_->context()->get_option("full_shape")) {
    return;
  }
  if (boost::all(*text, !boost::is_from_range('\x20', '\x7e'))) {
    return;
  }
  std::ostringstream oss;
  for (char ch : *text) {
    if (ch == 0x20) {
      oss << "\xe3\x80\x80";
    }
    else if (ch > 0x20 && ch <= 0x7e) {
      ch -= 0x20;
      oss << '\xef' << char('\xbc' + ch / 0x40) <<  char('\x80' + ch % 0x40);
    }
    else {
      oss << ch;
    }
  }
  *text = oss.str();
}

ProcessResult ShapeProcessor::ProcessKeyEvent(const KeyEvent& key_event) {
  DLOG(INFO) << "shape_processor: " << key_event;
  if (!engine_->context()->get_option("full_shape")) {
    return kNoop;
  }
  if (key_event.ctrl() || key_event.alt() || key_event.release()) {
    return kNoop;
  }
  int ch = key_event.keycode();
  if (ch < 0x20 || ch > 0x7e) {
    return kNoop;
  }
  string wide(1, ch);
  formatter_.Format(&wide);
  engine_->sink()(wide);
  return kAccepted;
}

}  // namespace rime
