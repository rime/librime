//
// Copyleft 2011 RIME Developers
// License: GPLv3
//
// 2012-12-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MESSENGER_H_
#define RIME_MESSENGER_H_

#include <string>
#include <boost/signals.hpp>
#include <rime/common.h>

namespace rime {

class Messenger {
 public:
  typedef boost::signal<void (const std::string& message_type,
                              const std::string& message_value)> MessageSink;
  
  MessageSink& message_sink() { return message_sink_; }
  
 protected:
  MessageSink message_sink_;
};

}  // namespace rime

#endif  // RIME_MESSENGER_H_
