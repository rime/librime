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

void RimeWith(const std::string &line) {
  // TODO: echo
  std::cout << line << std::endl;
}

int main(int argc, char *argv[]) {
  rime::RegisterComponents();
  // process input
  std::string line;
  while (std::cin) {
    std::getline(std::cin, line);
    RimeWith(line);
  }
  return 0;
}
