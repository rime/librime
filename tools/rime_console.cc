// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>

class Verse {
 public:
  Verse() : interactive_(false), engine_(new rime::Engine) {
    conn_ = engine_->sink().connect(boost::bind(&Verse::OnCommit, this, _1));
  }
  ~Verse() {
    conn_.disconnect();
  }

  void OnCommit(const std::string &commit_text) {
    EZLOGGERVAR(commit_text);
    std::cout << commit_text << std::endl;
  }

  void RimeWith(const std::string &line) {
    EZLOGGERVAR(line);
    rime::KeySequence keys;
    if (!keys.Parse(line)) {
      EZLOGGERPRINT("error parsing input: '%s'", line.c_str());
      return;
    }
    BOOST_FOREACH(const rime::KeyEvent &ke, keys) {
      engine_->ProcessKeyEvent(ke);
    }
    if (!interactive_) {
      rime::Context *ctx = engine_->context();
      if (ctx && ctx->IsComposing()) {
        ctx->Commit();
      }
    }
  }

  void set_interactive(bool interactive) { interactive_ = interactive; }
  bool interactive() const { return interactive_; }

 private:
  bool interactive_;
  rime::scoped_ptr<rime::Engine> engine_;
  boost::signals::connection conn_;
};

int main(int argc, char *argv[]) {
  // initialize la Rime
  rime::RegisterComponents();

  Verse verse;
  // "-i" turns on interactive mode (no commit at the end of line)
  bool interactive = argc > 1 && !strcmp(argv[1], "-i");
  verse.set_interactive(interactive);

  // process input
  std::string line;
  while (std::cin) {
    std::getline(std::cin, line);
    verse.RimeWith(line);
  }
  return 0;
}
