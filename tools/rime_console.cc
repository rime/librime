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
#include <rime/context.h>
#include <rime/deployer.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/switcher.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>

class RimeConsole {
 public:
  RimeConsole(rime::Schema *schema) : interactive_(false),
                                      engine_(new rime::Engine(schema)) {
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
    std::cout << "comp.  : [" << comp->GetDebugText() << "]" << std::endl;
    const rime::Segment &current(comp->back());
    if (!current.menu)
      return;
    int page_size = engine_->schema()->page_size();
    int page_no = current.selected_index / page_size;
    rime::scoped_ptr<rime::Page> page(
        current.menu->CreatePage(page_size, page_no));
    std::cout << "page_no: " << page_no
              << ", index: " << current.selected_index << std::endl;
    int i = 0;
    BOOST_FOREACH(const rime::shared_ptr<rime::Candidate> &cand,
                  page->candidates) {
      std::cout << "cand. " << (++i % 10) <<  ": [";
      std::cout << cand->text();
      std::cout << "]";
      if (!cand->comment().empty())
        std::cout << "  " << cand->comment();
      std::cout << std::endl;
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
  rime::RegisterComponents();

  rime::Deployer deployer;
  if (!deployer.InitializeInstallation()) {
    std::cerr << "failed to initialize installation." << std::endl;
    return 1;
  }

  rime::Switcher switcher;
  rime::Schema *schema = switcher.CreateSchema();
  if (schema) {
    std::cerr << "preparing dictionary for schema '" << schema->schema_id() << "'...";
    if (!deployer.InstallSchema(schema->schema_id() + ".schema.yaml")) {
      std::cerr << "failure!" << std::endl;
      delete schema;
      return 1;
    }
    std::cerr << "ready." << std::endl;
  }

  RimeConsole console(schema);
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
