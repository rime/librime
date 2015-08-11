//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2012-12-16 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_MESSENGER_H_
#define RIME_MESSENGER_H_

#include <rime/common.h>

namespace rime {

class Messenger {
 public:
  using MessageSink = signal<void (const string& message_type,
                                   const string& message_value)>;

  MessageSink& message_sink() { return message_sink_; }

 protected:
  MessageSink message_sink_;
};

}  // namespace rime

#endif  // RIME_MESSENGER_H_
