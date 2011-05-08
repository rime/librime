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
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/schema.h>

class RimeConsole {
 public:
  RimeConsole() : interactive_(false), engine_(new rime::Engine) {
    rime::Config *config = 
        rime::Config::Require("config")->Create("rime_console");
    engine_->set_schema(new rime::Schema("rime_console", config));
    conn_ = engine_->sink().connect(
        boost::bind(&RimeConsole::OnCommit, this, _1));
  }
  ~RimeConsole() {
    conn_.disconnect();
  }

  void OnCommit(const std::string &commit_text) {
    EZLOGGERVAR(commit_text);
    std::cout << commit_text << std::endl;
  }

  void ProcessLine(const std::string &line) {
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

// program entry
int main(int argc, char *argv[]) {
  // initialize la Rime
  rime::RegisterRimeComponents();

  RimeConsole console;
  // "-i" turns on interactive mode (no commit at the end of line)
  bool interactive = argc > 1 && !strcmp(argv[1], "-i");
  console.set_interactive(interactive);

  // process input
  std::string line;
  while (std::cin) {
    std::getline(std::cin, line);
    console.ProcessLine(line);
  }
  return 0;
}
