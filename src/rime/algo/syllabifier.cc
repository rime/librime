//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-07-12 Zou Xu <zouivex@gmail.com>
// 2012-02-11 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <queue>
#include <boost/range/adaptor/reversed.hpp>
#include <rime/algo/syllabifier.h>
#include <rime/dict/corrector.h>
#include <rime/dict/prism.h>
#include "syllabifier.h"

namespace rime {
using namespace corrector;

using Vertex = pair<size_t, SpellingType>;
using VertexQueue =
    std::priority_queue<Vertex, vector<Vertex>, std::greater<Vertex>>;

// 權重階梯：
// 1. 全拼：用戶完整輸入了所有編碼。Penalty = 0
// 2. 簡拼：用戶輸入了縮寫，明確希望匹配某個字。Penalty ≈ -2.3
// 3. 補全 (Completion)：用戶還沒打完，算法瞎猜的。Penalty ≈ -3.0
const double kCompletionPenalty = -2.995732273553991;      // log(0.05)
const double kCorrectionCredibility = -4.605170185988091;  // log(0.01)

// ── T9 增量音节图缓存：正确性要点（见 syllabifier.h 的 SyllableGraphCache）──
// 1. 图构建分四段，均为确定性的纯函数：BFS → 剪枝 → 补全 → Transpose。
//    缓存保存 BFS 段的输出（剪枝前），追加数字时在缓存图上续跑 BFS，
//    再对整图重跑剪枝与补全 —— 与整串全量重跑逐位一致。
// 2. BFS 逐位复现的归纳基础：输入前缀相同时，旧区域内每个顶点的
//    入边集合与首次弹出类型完全一致（darts CommonPrefixSearch 对更长
//    后缀的匹配是旧匹配的超集，旧区域内的边逐条重现），min-heap 按
//    (位置, 类型) 弹出，旧位置全部先于新位置被处理。因此把缓存顶点
//    按缓存类型重新入队扩展，即得到全量重跑的 BFS 输出。
// 3. 缓存准入：仅无 corrector（纠错边依赖查询时上下文）、非
//    strict_spelling（边属性依赖 input.length()）、输入全为数字
//    （分隔符跳转与输入长度相关，已由数字+无数字分隔符守卫排除）、
//    分隔符不含数字。其余输入（全拼等）走原始全量路径，零影响。
// 4. 缓存快照在补全之前：补全边每轮随输入末尾变化，绝不入缓存；
//    indices 存指针，缓存图中始终为空，每轮由 Transpose 重建。
// 5. prism 以 dict/schema 校验和为指纹：重新部署词典后指纹变化，
//    自动回退全量路径，不会拿旧图配新词典。

// 判断字符串是否全为数字（九键输入特征；全拼等字母输入不命中）
static bool IsAllDigits(const string& s) {
  if (s.empty())
    return false;
  for (char c : s) {
    if (c < '0' || c > '9')
      return false;
  }
  return true;
}

int Syllabifier::BuildSyllableGraph(const string& input,
                                    Prism& prism,
                                    SyllableGraph* graph) {
  return BuildSyllableGraph(input, prism, graph, nullptr);
}

int Syllabifier::BuildSyllableGraph(const string& input,
                                    Prism& prism,
                                    SyllableGraph* graph,
                                    SyllableGraphCache* cache) {
  if (input.empty())
    return 0;

  // ── T9 增量快速路径 ──
  // 条件：无 corrector + 非 strict_spelling + 输入全数字 + 缓存可扩展；
  // 任一条件不满足自动走全量路径，行为与旧版完全一致。
  if (cache && !corrector_ && !strict_spelling_ && IsAllDigits(input) &&
      TryExtendSyllableGraph(input, prism, graph, cache)) {
    return (int)graph->interpreted_length;
  }

  size_t farthest = 0;
  VertexQueue queue;
  queue.push(Vertex{0, kNormalSpelling});  // start

  while (!queue.empty()) {
    Vertex vertex(queue.top());
    queue.pop();
    size_t current_pos = vertex.first;

    // record a visit to the vertex
    if (graph->vertices.find(current_pos) == graph->vertices.end()) {
      graph->vertices.insert(vertex);  // preferred spelling type comes first
    } else {
      //      graph->vertices[current_pos] =
      //          std::min(vertex.second, graph->vertices[current_pos]);
      continue;  // discard worse spelling types
    }

    if (current_pos > farthest)
      farthest = current_pos;
    DLOG(INFO) << "current_pos: " << current_pos;

    // see where we can go by advancing a syllable
    vector<Prism::Match> matches;
    set<SyllableId> exact_match_syllables;
    auto current_input = input.substr(current_pos);
    prism.CommonPrefixSearch(current_input, &matches);
    if (corrector_) {
      for (auto& m : matches) {
        exact_match_syllables.insert(m.value);
      }
      Corrections corrections;
      corrector_->ToleranceSearch(prism, current_input, &corrections, 5);
      for (const auto& m : corrections) {
        for (auto accessor = prism.QuerySpelling(m.first);
             !accessor.exhausted(); accessor.Next()) {
          auto props = accessor.properties();
          if (props.type == kNormalSpelling && !props.is_correction) {
            matches.push_back({m.first, m.second.length});
            break;
          }
        }
      }
    }

    if (!matches.empty()) {
      auto& end_vertices(graph->edges[current_pos]);
      for (const auto& m : matches) {
        if (m.length == 0)
          continue;
        size_t end_pos = current_pos + m.length;
        // consume trailing delimiters
        while (end_pos < input.length() &&
               delimiters_.find(input[end_pos]) != string::npos)
          ++end_pos;
        DLOG(INFO) << "end_pos: " << end_pos;
        bool matches_input = (current_pos == 0 && end_pos == input.length());
        SpellingMap& spellings(end_vertices[end_pos]);
        SpellingType end_vertex_type = kInvalidSpelling;
        // when spelling algebra is enabled,
        // a spelling evaluates to a set of syllables;
        // otherwise, it resembles exactly the syllable itself.
        SpellingAccessor accessor(prism.QuerySpelling(m.value));
        while (!accessor.exhausted()) {
          SyllableId syllable_id = accessor.syllable_id();
          EdgeProperties props(accessor.properties());
          if (strict_spelling_ && matches_input &&
              props.type != kNormalSpelling) {
            // disqualify fuzzy spelling or abbreviation as single word
          } else {
            props.end_pos = end_pos;
            // add a syllable with properties to the edge's
            // spelling-to-syllable map
            if (corrector_ && exact_match_syllables.find(m.value) ==
                                  exact_match_syllables.end()) {
              props.is_correction = true;
              props.credibility = kCorrectionCredibility;
            }
            auto it = spellings.find(syllable_id);
            if (it == spellings.end()) {
              spellings.insert({syllable_id, props});
            } else {
              it->second.type = (std::min)(it->second.type, props.type);
            }
            // let end_vertex_type be the best (smaller) type of spelling
            // that ends at the vertex
            if (end_vertex_type > props.type && !props.is_correction) {
              end_vertex_type = props.type;
            }
          }
          accessor.Next();
        }
        if (spellings.empty()) {
          DLOG(INFO) << "not spelled.";
          end_vertices.erase(end_pos);
          continue;
        }
        // find the best common type in a path up to the end vertex
        // eg. pinyin "shurfa" has vertex type kNormalSpelling at position 3,
        // kAbbreviation at position 4 and kAbbreviation at position 6
        if (end_vertex_type < vertex.second) {
          end_vertex_type = vertex.second;
        }
        queue.push(Vertex{end_pos, end_vertex_type});
        DLOG(INFO) << "added to syllable graph, edge: [" << current_pos << ", "
                   << end_pos << ")";
      }
    }
  }

  // ── T9 增量缓存：保存 BFS 段输出（剪枝/补全前）的图快照 ──
  // 快照必须取剪枝与补全之前：剪枝会删除顶点/边、补全边随输入末尾变化，
  // 二者每轮全量重跑，不入缓存。此时 Transpose 未执行，indices 为空。
  if (cache) {
    StoreSyllableGraphCache(input, prism, *graph, farthest, cache);
  }

  DLOG(INFO) << "remove stale vertices and edges";
  set<int> good;
  good.insert(farthest);
  // fuzzy spellings are immune to invalidation by normal spellings
  SpellingType last_type =
      (std::max)(graph->vertices[farthest], kFuzzySpelling);
  for (int i = farthest - 1; i >= 0; --i) {
    if (graph->vertices.find(i) == graph->vertices.end())
      continue;
    // remove stale edges
    for (auto j = graph->edges[i].begin(); j != graph->edges[i].end();) {
      if (good.find(j->first) == good.end()) {
        // not connected
        graph->edges[i].erase(j++);
        continue;
      }
      // remove disqualified syllables (eg. matching abbreviated spellings)
      // when there is a path of more favored type
      SpellingType edge_type = kInvalidSpelling;
      for (auto k = j->second.begin(); k != j->second.end();) {
        if (k->second.is_correction) {
          ++k;
          continue;  // Don't care correction edges
        }
        if (k->second.type > last_type) {
          j->second.erase(k++);
        } else {
          if (k->second.type < edge_type)
            edge_type = k->second.type;
          ++k;
        }
      }
      if (j->second.empty()) {
        graph->edges[i].erase(j++);
      } else {
        if (edge_type < kAbbreviation)
          CheckOverlappedSpellings(graph, i, j->first);
        ++j;
      }
    }
    if (graph->vertices[i] > last_type || graph->edges[i].empty()) {
      DLOG(INFO) << "remove stale vertex at " << i;
      graph->vertices.erase(i);
      graph->edges.erase(i);
      continue;
    }
    // keep the valid vertex
    good.insert(i);
  }

  if (enable_completion_ && farthest < input.length()) {
    DLOG(INFO) << "completion enabled";
    const size_t kExpandSearchLimit = 512;
    vector<Prism::Match> keys;
    prism.ExpandSearch(input.substr(farthest), &keys, kExpandSearchLimit);
    if (!keys.empty()) {
      size_t current_pos = farthest;
      size_t end_pos = input.length();
      size_t code_length = end_pos - current_pos;
      auto& end_vertices(graph->edges[current_pos]);
      auto& spellings(end_vertices[end_pos]);
      for (const auto& m : keys) {
        if (m.length < code_length)
          continue;
        // when spelling algebra is enabled,
        // a spelling evaluates to a set of syllables;
        // otherwise, it resembles exactly the syllable itself.
        SpellingAccessor accessor(prism.QuerySpelling(m.value));
        while (!accessor.exhausted()) {
          SyllableId syllable_id = accessor.syllable_id();
          SpellingProperties props = accessor.properties();
          if (props.type < kAbbreviation) {
            props.type = kCompletion;
            props.credibility += kCompletionPenalty;
            props.end_pos = end_pos;
            // add a syllable with properties to the edge's
            // spelling-to-syllable map
            spellings.insert({syllable_id, props});
          }
          accessor.Next();
        }
      }
      if (spellings.empty()) {
        DLOG(INFO) << "no completion could be made.";
        end_vertices.erase(end_pos);
      } else {
        DLOG(INFO) << "added to syllable graph, completion: [" << current_pos
                   << ", " << end_pos << ")";
        farthest = end_pos;
      }
    }
  }

  graph->input_length = input.length();
  graph->interpreted_length = farthest;
  DLOG(INFO) << "input length: " << graph->input_length;
  DLOG(INFO) << "syllabified length: " << graph->interpreted_length;

  Transpose(graph);

  return farthest;
}

void Syllabifier::StoreSyllableGraphCache(const string& input,
                                          Prism& prism,
                                          const SyllableGraph& graph,
                                          size_t farthest,
                                          SyllableGraphCache* cache) {
  // 仅缓存纯数字输入（九键）；corrector/strict_spelling 会引入跨轮不稳的
  // 边属性；分隔符含数字时，追加数字会改变分隔符跳转的边界，均不入缓存。
  if (!IsAllDigits(input) || corrector_ || strict_spelling_ ||
      delimiters_.find_first_of("0123456789") != string::npos)
    return;
  cache->input = input;
  cache->farthest = farthest;
  cache->graph = graph;
  cache->graph.indices.clear();  // indices 存指针，禁止跨轮携带
  cache->dict_checksum = prism.dict_file_checksum();
  cache->schema_checksum = prism.schema_file_checksum();
  cache->delimiters = delimiters_;
  cache->enable_completion = enable_completion_;
  cache->strict_spelling = strict_spelling_;
  cache->valid = true;
}

bool Syllabifier::TryExtendSyllableGraph(const string& input,
                                         Prism& prism,
                                         SyllableGraph* graph,
                                         SyllableGraphCache* cache) {
  const size_t old_len = cache->input.size();
  if (!cache->valid || old_len == 0 || input.size() <= old_len ||
      input.compare(0, old_len, cache->input) != 0 ||
      cache->graph.vertices.find(cache->farthest) ==
          cache->graph.vertices.end() ||
      cache->dict_checksum != prism.dict_file_checksum() ||
      cache->schema_checksum != prism.schema_file_checksum() ||
      cache->delimiters != delimiters_ ||
      cache->enable_completion != enable_completion_ ||
      cache->strict_spelling != strict_spelling_)
    return false;

  DLOG(INFO) << "T9 syllable graph incremental: '" << cache->input
             << "' -> '" << input << "'";
  // 复用 BFS 段输出的缓存图；indices 为空，扩展结束后重建。
  *graph = cache->graph;
  VertexQueue queue;
  // 把缓存全部顶点按缓存类型重新入队扩展：BFS 首次弹出类型即缓存类型
  // （归纳基础见文件头注释），重新扩展只为发现终点越过旧输入末尾的新边。
  for (const auto& v : graph->vertices) {
    queue.push(Vertex{v.first, v.second});
  }
  size_t farthest = 0;
  set<size_t> expanded;  // 旧顶点已在 vertices 中，用独立集合判重
  while (!queue.empty()) {
    Vertex vertex(queue.top());
    queue.pop();
    size_t current_pos = vertex.first;

    if (graph->vertices.find(current_pos) == graph->vertices.end()) {
      graph->vertices.insert(vertex);
    } else if (expanded.find(current_pos) != expanded.end()) {
      continue;  // 重复（更差类型）候选，与全量路径一致地丢弃
    }
    expanded.insert(current_pos);

    if (current_pos > farthest)
      farthest = current_pos;

    vector<Prism::Match> matches;
    auto current_input = input.substr(current_pos);
    prism.CommonPrefixSearch(current_input, &matches);
    if (matches.empty()) {
      continue;
    }
    auto& end_vertices(graph->edges[current_pos]);
    for (const auto& m : matches) {
      if (m.length == 0)
        continue;
      size_t end_pos = current_pos + m.length;
      // consume trailing delimiters（数字输入 + 分隔符不含数字，不触发）
      while (end_pos < input.length() &&
             delimiters_.find(input[end_pos]) != string::npos)
        ++end_pos;
      SpellingMap& spellings(end_vertices[end_pos]);
      SpellingType end_vertex_type = kInvalidSpelling;
      SpellingAccessor accessor(prism.QuerySpelling(m.value));
      // corrector_ / strict_spelling_ 均已在缓存准入条件中排除，
      // 故省略全量路径中的纠错与 matches_input 分支。
      while (!accessor.exhausted()) {
        SyllableId syllable_id = accessor.syllable_id();
        EdgeProperties props(accessor.properties());
        props.end_pos = end_pos;
        auto it = spellings.find(syllable_id);
        if (it == spellings.end()) {
          spellings.insert({syllable_id, props});
        } else {
          it->second.type = (std::min)(it->second.type, props.type);
        }
        if (end_vertex_type > props.type) {
          end_vertex_type = props.type;
        }
        accessor.Next();
      }
      if (spellings.empty()) {
        end_vertices.erase(end_pos);
        continue;
      }
      if (end_vertex_type < vertex.second) {
        end_vertex_type = vertex.second;
      }
      queue.push(Vertex{end_pos, end_vertex_type});
    }
  }

  // 保存本轮 BFS 段输出（剪枝/补全前）快照，供下一轮扩展
  StoreSyllableGraphCache(input, prism, *graph, farthest, cache);

  // 与全量路径相同的剪枝（锚定新的 farthest，整图重跑）
  DLOG(INFO) << "remove stale vertices and edges (incremental)";
  set<int> good;
  good.insert(farthest);
  SpellingType last_type =
      (std::max)(graph->vertices[farthest], kFuzzySpelling);
  for (int i = farthest - 1; i >= 0; --i) {
    if (graph->vertices.find(i) == graph->vertices.end())
      continue;
    for (auto j = graph->edges[i].begin(); j != graph->edges[i].end();) {
      if (good.find(j->first) == good.end()) {
        graph->edges[i].erase(j++);
        continue;
      }
      SpellingType edge_type = kInvalidSpelling;
      for (auto k = j->second.begin(); k != j->second.end();) {
        if (k->second.is_correction) {
          ++k;
          continue;
        }
        if (k->second.type > last_type) {
          j->second.erase(k++);
        } else {
          if (k->second.type < edge_type)
            edge_type = k->second.type;
          ++k;
        }
      }
      if (j->second.empty()) {
        graph->edges[i].erase(j++);
      } else {
        if (edge_type < kAbbreviation)
          CheckOverlappedSpellings(graph, i, j->first);
        ++j;
      }
    }
    if (graph->vertices[i] > last_type || graph->edges[i].empty()) {
      graph->vertices.erase(i);
      graph->edges.erase(i);
      continue;
    }
    good.insert(i);
  }

  if (enable_completion_ && farthest < input.length()) {
    const size_t kExpandSearchLimit = 512;
    vector<Prism::Match> keys;
    prism.ExpandSearch(input.substr(farthest), &keys, kExpandSearchLimit);
    if (!keys.empty()) {
      size_t current_pos = farthest;
      size_t end_pos = input.length();
      size_t code_length = end_pos - current_pos;
      auto& end_vertices(graph->edges[current_pos]);
      auto& spellings(end_vertices[end_pos]);
      for (const auto& m : keys) {
        if (m.length < code_length)
          continue;
        SpellingAccessor accessor(prism.QuerySpelling(m.value));
        while (!accessor.exhausted()) {
          SyllableId syllable_id = accessor.syllable_id();
          SpellingProperties props = accessor.properties();
          if (props.type < kAbbreviation) {
            props.type = kCompletion;
            props.credibility += kCompletionPenalty;
            props.end_pos = end_pos;
            spellings.insert({syllable_id, props});
          }
          accessor.Next();
        }
      }
      if (spellings.empty()) {
        end_vertices.erase(end_pos);
      } else {
        farthest = end_pos;
      }
    }
  }

  graph->input_length = input.length();
  graph->interpreted_length = farthest;
  graph->indices.clear();
  Transpose(graph);
  return true;
}

void Syllabifier::CheckOverlappedSpellings(SyllableGraph* graph,
                                           size_t start,
                                           size_t end) {
  if (!graph || graph->edges.find(start) == graph->edges.end())
    return;
  // if "Z" = "YX", mark the vertex between Y and X an ambiguous syllable joint
  auto& y_end_vertices(graph->edges[start]);
  // enumerate Ys
  for (const auto& y : y_end_vertices) {
    size_t joint = y.first;
    if (joint >= end)
      break;
    // test X
    if (graph->edges.find(joint) == graph->edges.end())
      continue;
    auto& x_end_vertices(graph->edges[joint]);
    for (auto& x : x_end_vertices) {
      if (x.first < end)
        continue;
      if (x.first == end) {
        // discourage syllables at an ambiguous joint
        // bad cases include pinyin syllabification "niju'ede"
        for (auto& spelling : x.second) {
          // 這條邊（X）相對於起點構成歧義
          spelling.second.ambiguous_source_positions.insert(start);
        }
        graph->vertices[joint] = kAmbiguousSpelling;
        DLOG(INFO) << "ambiguous syllable joint at position " << joint << ".";
      }
      break;
    }
  }
}

void Syllabifier::Transpose(SyllableGraph* graph) {
  for (const auto& start : graph->edges) {
    auto& index(graph->indices[start.first]);
    for (const auto& end : boost::adaptors::reverse(start.second)) {
      for (const auto& spelling : end.second) {
        SyllableId syll_id = spelling.first;
        index[syll_id].push_back(&spelling.second);
      }
    }
  }
}

void Syllabifier::EnableCorrection(Corrector* corrector) {
  corrector_ = corrector;
}

}  // namespace rime
