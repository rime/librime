//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <cstring>
#include <iostream>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/composition.h>
#include <rime/context.h>
#include <rime/deployer.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/menu.h>
#include <rime/schema.h>
#include <rime/setup.h>
#include <rime/dict/dictionary.h>
#include <rime/dict/dict_compiler.h>
#include <rime/lever/deployment_tasks.h>

using namespace rime;

class RimeConsole {
 public:
  RimeConsole() : interactive_(false),
                  engine_(Engine::Create()) {
    conn_ = engine_->sink().connect(
        [this](const string& x) { OnCommit(x); });
  }
  ~RimeConsole() {
    conn_.disconnect();
  }

  void OnCommit(const string &commit_text) {
    if (interactive_) {
      std::cout << "commit : [" << commit_text << "]" << std::endl;
    }
    else {
      std::cout << commit_text << std::endl;
    }
  }

  void PrintComposition(const Context *ctx) {
    if (!ctx || !ctx->IsComposing())
      return;
    std::cout << "input  : [" << ctx->input() << "]" << std::endl;
    const Composition &comp = ctx->composition();
    if (comp.empty())
      return;
    std::cout << "comp.  : [" << comp.GetDebugText() << "]" << std::endl;
    const Segment &current(comp.back());
    if (!current.menu)
      return;
    int page_size = engine_->schema()->page_size();
    int page_no = current.selected_index / page_size;
    the<Page> page(current.menu->CreatePage(page_size, page_no));
    if (!page)
      return;
    std::cout << "page_no: " << page_no
              << ", index: " << current.selected_index << std::endl;
    int i = 0;
    for (const an<Candidate> &cand : page->candidates) {
      std::cout << "cand. " << (++i % 10) <<  ": [";
      std::cout << cand->text();
      std::cout << "]";
      if (!cand->comment().empty())
        std::cout << "  " << cand->comment();
      std::cout << "  quality=" << cand->quality();
      std::cout << std::endl;
    }
  }

  void ProcessLine(const string &line) {
    KeySequence keys;
    if (!keys.Parse(line)) {
      LOG(ERROR) << "error parsing input: '" << line << "'";
      return;
    }
    for (const KeyEvent &key : keys) {
      engine_->ProcessKey(key);
    }
    Context *ctx = engine_->active_context();
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
  the<Engine> engine_;
  connection conn_;
};

// program entry
int main(int argc, char *argv[]) {
  // initialize la Rime
  SetupLogging("rime.console");
  LoadModules(kDefaultModules);

  Deployer deployer;
  InstallationUpdate installation;
  if (!installation.Run(&deployer)) {
    std::cerr << "failed to initialize installation." << std::endl;
    return 1;
  }
  std::cerr << "initializing...";
  WorkspaceUpdate workspace_update;
  if (!workspace_update.Run(&deployer)) {
    std::cerr << "failure!" << std::endl;
    return 1;
  }
  std::cerr << "ready." << std::endl;

  RimeConsole console;
  // "-i" turns on interactive mode (no commit at the end of line)
  bool interactive = argc > 1 && !strcmp(argv[1], "-i");
  console.set_interactive(interactive);

  // process input
  string line;
  while (std::cin) {
    std::getline(std::cin, line);
    console.ProcessLine(line);
  }
  return 0;
}
