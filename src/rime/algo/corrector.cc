//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/14.
//

#include "corrector.h"

using namespace rime;

void DFSCollect(const string &origin, const string &deleted, size_t ed, Script &result);

Script CorrectionCollector::Collect(size_t edit_distance) {
  Script script;

  for (auto &v : syllabary_) {
    DFSCollect(v, v, edit_distance, script);
  }

  return script;
}

void DFSCollect(const string &origin, const string &deleted, size_t ed, Script &result) {
  if (ed <= 0) return;
  for (size_t i = 0; i < deleted.size(); i++) {
    string temp = deleted;
    temp.erase(i, 1);
    Spelling spelling(origin);
    spelling.properties.type = kCorrection;
    result[temp].push_back(spelling);
    DFSCollect(origin, temp, ed - 1, result);
  }
}