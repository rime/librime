/*
 * Copyright RIME Developers
 * Distributed under the BSD License
 */
#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "line_editor.h"

class CompletionCatalog {
 public:
  CompletionCatalog();

  const std::vector<LineCompletion>& items() const { return items_; }

 private:
  void AddCommand(const std::string& command);
  void AddKey(const std::string& key);
  void Add(const std::string& text,
           const std::string& display,
           const std::string& detail,
           const std::string& kind);

  std::vector<LineCompletion> items_;
  std::unordered_set<std::string> seen_;
};
