//
// Copyright RIME Developers
// Distributed under the BSD License
//
// Created by nameoverflow on 2018/11/14.
//

#include <numeric>
#include <algorithm>
#include <queue>
#include "corrector.h"

using namespace rime;

static hash_map<char, hash_set<char>> keyboard_map = {
    {'1', {'2', 'q', 'w'}},
    {'2', {'1', '3', 'q', 'w', 'e'}},
    {'3', {'2', '4', 'w', 'e', 'r'}},
    {'4', {'3', '5', 'e', 'r', 't'}},
    {'5', {'4', '6', 'r', 't', 'y'}},
    {'6', {'5', '7', 't', 'y', 'u'}},
    {'7', {'6', '8', 'y', 'u', 'i'}},
    {'8', {'7', '9', 'u', 'i', 'o'}},
    {'9', {'8', '0', 'i', 'o', 'p'}},
    {'0', {'9', '-', 'o', 'p', '['}},
    {'-', {'0', '=', 'p', '[', ']'}},
    {'=', {'-', '[', ']', '\\'}},
    {'q', {'w'}},
    {'w', {'q', 'e'}},
    {'e', {'w', 'r'}},
    {'r', {'e', 't'}},
    {'t', {'r', 'y'}},
    {'y', {'t', 'u'}},
    {'u', {'y', 'i'}},
    {'i', {'u', 'o'}},
    {'o', {'i', 'p'}},
    {'p', {'o', '['}},
    {'[', {'p', ']'}},
    {']', {'[', '\\'}},
    {'\\', {']'}},
    {'a', {'s'}},
    {'s', {'a', 'd'}},
    {'d', {'s', 'f'}},
    {'f', {'d', 'g'}},
    {'g', {'f', 'h'}},
    {'h', {'g', 'j'}},
    {'j', {'h', 'k'}},
    {'k', {'j', 'l'}},
    {'l', {'k', ';'}},
    {';', {'l', '\''}},
    {'\'', {';'}},
    {'z', {'x'}},
    {'x', {'z', 'c'}},
    {'c', {'x', 'v'}},
    {'v', {'c', 'b'}},
    {'b', {'v', 'n'}},
    {'n', {'b', 'm'}},
    {'m', {'n', ','}},
    {',', {'m', '.'}},
    {'.', {',', '/'}},
    {'/', {'.'}},
};

void DFSCollect(const string &origin, const string &current, size_t ed, Script &result);

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

vector<Prism::Match> Corrector::SymDeletePrefixSearch(const string& key) {
  if (key.empty())
    return {};
  size_t key_len = key.length();
  size_t prepared_size = key_len * (key_len - 1);

  vector<Match> result;
  result.reserve(prepared_size);
  vector<size_t> jump_pos(key_len);

  auto match_next = [&](size_t &node, size_t &point) -> bool {
    auto res_val = trie_->traverse(key.c_str(), node, point, point + 1);
    if (res_val == -2) return false;
    if (res_val >= 0) {
      result.push_back({ res_val, point });
    }
    return true;
  };

  // pass through origin key, cache trie nodes
  size_t max_match = 0;
  for (size_t next_node = 0; max_match < key_len;) {
    jump_pos[max_match] = next_node;
    if (!match_next(next_node, max_match)) break;
  }

  // start at the next position of deleted char
  for (size_t del_pos = 0; del_pos <= max_match; del_pos++) {
    size_t next_node = jump_pos[del_pos];
    for (size_t key_point = del_pos + 1; key_point < key_len;) {
      if (!match_next(next_node, key_point)) break;
    }
  }

  return result;
}


inline uint8_t SubstCost(char left, char right) {
  if (left == right) return 0;
  if (keyboard_map[left].find(right) != keyboard_map[left].end()) {
    return 1;
  }
  return 4;
}

// This nice O(min(m, n)) implementation is from
// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
uint8_t Corrector::LevenshteinDistance(const std::string &s1, const std::string &s2) {
  // To change the type this function manipulates and returns, change
  // the return type and the types of the two variables below.
  auto s1len = (size_t)s1.size();
  auto s2len = (size_t)s2.size();

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
          last_diagonal + SubstCost(s1[y - 1], s2[x - 1])
      };

      column[y] = std::min(possibilities);
      last_diagonal = old_diagonal;
    }
  }
  auto result = column[s1len];
  delete[] column;
  return result;
}

// L's distance with transposition allowed
uint8_t Corrector::RestrictedDistance(const std::string& s1, const std::string& s2)
{
  auto len1 = s1.size(), len2 = s2.size();
  vector<size_t> d((len1 + 1) * (len2 + 1));
//  size_t d[len1 + 1][len2 + 1];

  auto index = [len1, len2](size_t i, size_t j) {
    return i * (len2 + 1) + j;
  };

  d[0] = 0;
  for(size_t i = 1; i <= len1; ++i) d[index(i, 0)] = i * 2;
  for(size_t i = 1; i <= len2; ++i) d[index(0, i)] = i * 2;

  for(size_t i = 1; i <= len1; ++i)
    for(size_t j = 1; j <= len2; ++j) {
      d[index(i, j)] = std::min({
                              d[index(i - 1, j)] + 2,
                              d[index(i, j - 1)] + 2,
                              d[index(i - 1, j - 1)] + SubstCost(s1[i - 1], s2[j - 1])
                          });
      if (i > 1 && j > 1 && s1[i - 2] == s2[j - 1] && s1[i - 1] == s2[j - 2]) {
        d[index(i, j)] = std::min(d[index(i, j)], d[index(i - 2, j - 2)] + 2);
      }
    }
  return (uint8_t)d[index(len1, len2)];
}
bool Corrector::Build(const Syllabary &syllabary,
                      const Script *script,
                      uint32_t dict_file_checksum,
                      uint32_t schema_file_checksum) {
  Syllabary correct_syllabary;
  if (script && !script->empty()) {
    for (auto &v : *script) {
      correct_syllabary.insert(v.first);
    }
  } else {
    correct_syllabary = syllabary;
  }

  CorrectionCollector collector(correct_syllabary);
  auto correction_script = collector.Collect((size_t)1);

  return Prism::Build(syllabary, &correction_script, dict_file_checksum, schema_file_checksum);
}

vector<NearSearchCorrector::Correction>
NearSearchCorrector::ToleranceSearch(const Prism& prism, const string &key, size_t tolerance) {
  vector<Correction> result;
  if (key.empty())
    return result;

  using record = struct {
    size_t node_pos;
    size_t idx;
    size_t distance;
    char ch;
  };

  std::queue<record> queue;
  queue.push({ 0, 0, 0, key[0] });
  for (auto subst : keyboard_map[key[0]]) {
    queue.push({ 0, 0, 1, subst });
  }
  for (; !queue.empty(); queue.pop()) {
    auto &rec = queue.front();
    char ch = rec.ch;
    std::swap(ch, const_cast<char *>(key.c_str())[rec.idx]);
    auto val = prism.trie().traverse(key.c_str(), rec.node_pos, rec.idx, rec.idx + 1);
    std::swap(ch, const_cast<char *>(key.c_str())[rec.idx - 1]);

    if (val == -2) continue;
    if (val >= 0) {
      result.push_back({ rec.distance, val, rec.idx });
    }
    if (rec.idx < key.size()) {
      queue.push({ rec.node_pos, rec.idx, rec.distance, key[rec.idx] });
      if (rec.distance < tolerance) {
        for (auto subst : keyboard_map[key[rec.idx]]) {
          queue.push({ rec.node_pos, rec.idx, rec.distance + 1, subst });
        }
      }
    }
  }
  return result;
}
