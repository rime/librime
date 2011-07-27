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
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/impl/dictionary.h>

class RimeConsole {
 public:
  static const int kPageSize = 9;
  
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
    if (interactive_) {
      std::cout << "commit : [" << commit_text << "]" << std::endl;
    }
    else {
      std::cout << commit_text << std::endl;
    }
  }

  void PrintComposition(const rime::Context *ctx) {
    EZLOGGERFUNCTRACKER;
    if (!ctx || !ctx->IsComposing())
      return;
    std::cout << "input  : [" << ctx->input() << "]" << std::endl;
    const rime::Composition *comp = ctx->composition();
    if (!comp || comp->empty())
      return;
    int i = 0;
    std::cout << "comp.  : [";
    BOOST_FOREACH(const rime::Segment &seg, *comp) {
      if (i++ > 0)
        std::cout << "|";
      rime::shared_ptr<const rime::Candidate> cand = 
          seg.GetSelectedCandidate();
      if (cand)
        std::cout << cand->text();
    }
    std::cout << "]" << std::endl;
    const rime::Segment &current(comp->back());
    int page_no = current.selected_index / kPageSize;
    rime::scoped_ptr<rime::Page> page(
        current.menu->CreatePage(kPageSize, page_no));
    std::cout << "page_no: " << page_no
              << ", index: " << current.selected_index << std::endl;
    i = 0;
    BOOST_FOREACH(const rime::shared_ptr<rime::Candidate> &cand,
                  page->candidates) {
      std::cout << "cand. " << ++i <<  ": [";
      std::cout << cand->text();
      std::cout << "]" << std::endl;
    }
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
    rime::Context *ctx = engine_->context();
    if (interactive_) {
      PrintComposition(ctx);
    }
    else {
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
  // preparing dictionary
  {
    rime::Dictionary dict("luna_pinyin");
    if (!dict.Exists()) {
      dict.Compile(dict.name() + ".dict.yaml");
    }
  }
  
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
