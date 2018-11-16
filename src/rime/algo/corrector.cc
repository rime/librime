//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/14.
//

#include <numeric>
#include "corrector.h"

using namespace rime;

void DFSCollect(const string &origin, const string &deleted, size_t ed, Script &result);
uint16_t LevenshteinDistance(const std::string &s1, const std::string &s2);

Script CorrectionCollector::Collect(size_t edit_distance) {
  // TODO: specifically for 1 length str
  Script script;

  for (auto &v : syllabary_) {
    DFSCollect(v, v, edit_distance, script);
  }

  return script;
}

void DFSCollect(const string &origin, const string &current, size_t ed, Script &result) {
  if (ed <= 0) return;
  for (size_t i = 0; i < current.size(); i++) {
    string temp = current;
    temp.erase(i, 1);
    Spelling spelling(origin);
    spelling.properties.type = kCorrection;
    spelling.properties.tips = origin;
    result[temp].push_back(spelling);
    DFSCollect(origin, temp, ed - 1, result);
  }
}

optional<Corrector::Corrections> Corrector::SymDeletePrefixSearch(const string& key) {
  if (key.empty())
    return boost::none;
  size_t key_len = key.length();
  size_t prepared_size = key_len * (key_len - 1);

  Corrections result;
  result.reserve(prepared_size);
  vector<size_t> jump_pos(key_len);

  auto match_next = [&](size_t &node, size_t &point, function<void(SyllableId)> success) -> bool {
    auto res_val = trie_->traverse(key.c_str(), node, point, point + 1);
    if (res_val == -2) return false;
    if (res_val >= 0) {
      success(res_val);
    }
    return true;
  };

  // pass through origin key, cache trie nodes
  size_t max_match = 0;
  for (size_t next_node = 0; max_match < key_len;) {
    jump_pos[max_match] = next_node;
    if (!match_next(next_node, max_match, [&](SyllableId res) {
      result.push_back({ 1, res });
    })) break;
  }

  // start at the next position of deleted char
  for (size_t del_pos = 0; del_pos < max_match; del_pos++) {
    size_t next_node = jump_pos[del_pos];
    for (size_t key_point = del_pos + 1; key_point < key_len;) {
      if (!match_next(next_node, key_point, [&](SyllableId res) {
        auto distance = LevenshteinDistance(
            key.substr(0, key_point),
            key.substr(0, del_pos) + key.substr(del_pos + 1, key_point - del_pos - 1)
          );
        result.push_back({ distance, res });
      })) break;
    }
  }

  return result;
}

// This nice O(min(m, n)) implementation is from
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
uint16_t LevenshteinDistance(const std::string &s1, const std::string &s2) {
  // To change the type this function manipulates and returns, change
  // the return type and the types of the two variables below.
  auto s1len = (uint16_t)s1.size();
  auto s2len = (uint16_t)s2.size();

  auto column_start = (decltype(s1len))1;

  auto column = new decltype(s1len)[s1len + 1];
  std::iota(column + column_start - 1, column + s1len + 1, column_start - 1);

  for (auto x = column_start; x <= s2len; x++) {
    column[0] = x;
    auto last_diagonal = x - column_start;
    for (auto y = column_start; y <= s1len; y++) {
      auto old_diagonal = column[y];
      auto possibilities = {
          column[y] + 1,
          column[y - 1] + 1,
          last_diagonal + (s1[y - 1] == s2[x - 1]? 0 : 1)
      };
      column[y] = std::min(possibilities);
      last_diagonal = old_diagonal;
    }
  }
  auto result = column[s1len];
  delete[] column;
  return result;
}
