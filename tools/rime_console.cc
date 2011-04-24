// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-24 GONG Chen <chen.sst@gmail.com>
//
#include <iostream>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/engine.h>
#include <rime/key_event.h>

class Verse {
 public:
  void RimeWith(const std::string &line) {
    rime::KeySequence ks;
    if (!ks.Parse(line)) {
      EZLOGGERPRINT("error parsing input: %s", line.c_str());
    }
    // TODO:
    std::cout << ks.repr() << std::endl;
  }

 private:
  rime::scoped_ptr<rime::Engine> engine_;
};

int main(int argc, char *argv[]) {
  rime::RegisterComponents();
  Verse verse;
  std::string line;
  // process input
  while (std::cin) {
    std::getline(std::cin, line);
    verse.RimeWith(line);
  }
  return 0;
}
