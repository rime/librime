//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-7-12 Zou xu <zouivex@gmail.com>
//

#ifndef RIME_SYLLABIFIER_H_
#define RIME_SYLLABIFIER_H_

#include <stdint.h>
#include <rime_api.h>
#include <rime/common.h>
#include "spelling.h"

namespace rime {

class Prism;
class Corrector;

using SyllableId = int32_t;

struct EdgeProperties : SpellingProperties {
  EdgeProperties(SpellingProperties sup) : SpellingProperties(sup) {};
  EdgeProperties() = default;
  // 切分歧義編碼段的起始位置
  set<size_t> ambiguous_source_positions;
};

using SpellingMap = map<SyllableId, EdgeProperties>;
using VertexMap = map<size_t, SpellingType>;
using EndVertexMap = map<size_t, SpellingMap>;
using EdgeMap = map<size_t, EndVertexMap>;

using SpellingPropertiesList = vector<const EdgeProperties*>;
using SpellingIndex = map<SyllableId, SpellingPropertiesList>;
using SpellingIndices = map<size_t, SpellingIndex>;

struct SyllableGraph {
  size_t input_length = 0;
  size_t interpreted_length = 0;
  VertexMap vertices;
  EdgeMap edges;
  SpellingIndices indices;
};

// ── T9（九键）增量音节图缓存 ──
// 纯数字输入逐位增长时（九键快速输入），音节图构建是
// (input, prism, 选项) 的纯函数：BFS 阶段对旧前缀逐位复现一致。
// 缓存保存「BFS 后、剪枝前」的图（BFS 输出即全量重跑在同一前缀上
// 会得到的状态），下一次严格追加时把缓存全部顶点按缓存类型重新入队
// 扩展，再全量重跑剪枝与补全，结果与整串全量重跑完全一致
// （正确性要点见 syllabifier.cc 实现注释）。
// 必须由调用方（ScriptTranslator，每 session 一个）持有并传入，
// 禁止做成全局静态：不同 session/engine 的 prism 与选项不同，会互相污染。
struct SyllableGraphCache {
  string input;   // 缓存图对应的完整输入串
  size_t farthest = 0;  // BFS 阶段（剪枝/补全前）的最远解释位置
  bool valid = false;   // 缓存是否有效（构建环境受限时为 false）
  uint32_t dict_checksum = 0;    // prism 内容指纹，防跨词典/重部署误用
  uint32_t schema_checksum = 0;
  string delimiters;
  bool enable_completion = false;
  bool strict_spelling = false;
  SyllableGraph graph;  // 缓存图（indices 始终为空，扩展后由 Transpose 重建）
};

class Syllabifier {
 public:
  Syllabifier() = default;
  explicit Syllabifier(const string& delimiters,
                       bool enable_completion = false,
                       bool strict_spelling = false)
      : delimiters_(delimiters),
        enable_completion_(enable_completion),
        strict_spelling_(strict_spelling) {}

  RIME_DLL int BuildSyllableGraph(const string& input,
                                  Prism& prism,
                                  SyllableGraph* graph);
  // 传入非空 cache 时，纯数字输入可复用上一轮的音节图做尾部增量扩展；
  // cache 中的环境指纹不匹配或图不可扩展时自动回退全量路径并更新缓存。
  RIME_DLL int BuildSyllableGraph(const string& input,
                                  Prism& prism,
                                  SyllableGraph* graph,
                                  SyllableGraphCache* cache);
  RIME_DLL void EnableCorrection(Corrector* corrector);

 protected:
  bool TryExtendSyllableGraph(const string& input,
                              Prism& prism,
                              SyllableGraph* graph,
                              SyllableGraphCache* cache);
  void StoreSyllableGraphCache(const string& input,
                               Prism& prism,
                               const SyllableGraph& graph,
                               size_t farthest,
                               SyllableGraphCache* cache);
  void CheckOverlappedSpellings(SyllableGraph* graph, size_t start, size_t end);
  void Transpose(SyllableGraph* graph);

  string delimiters_;
  bool enable_completion_ = false;
  bool strict_spelling_ = false;
  Corrector* corrector_ = nullptr;
};

}  // namespace rime

#endif  // RIME_SYLLABIFIER_H_
