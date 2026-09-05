//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//

#ifndef RIME_POET_H_
#define RIME_POET_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/gear/translator_commons.h>
#include <rime/gear/contextual_translation.h>

namespace rime {

using WordGraph = map<int, map<int, DictEntryList>>;

class Grammar;
class Language;
struct Line;

class Poet {
 public:
  // Line "less", used to compare composed line of the same input range.
  using Compare = function<bool(const Line&, const Line&)>;

  static bool CompareWeight(const Line& one, const Line& other);
  static bool LeftAssociateCompare(const Line& one, const Line& other);

  // 前向搜索的中间状态（beam states），用于 T9 增量造句。
  // 对调用方不透明：MakeSentence 把状态存入 lattice，
  // MakeSentenceIncremental 复用并就地推进。
  // Lattice 中的 Line 指向传入 WordGraph 的词条与 lattice 内部前驱，
  // 复用时必须传入同一个（由调用方缓存的）WordGraph 对象。
  class Lattice {
   public:
    Lattice() = default;
    virtual ~Lattice() = default;

    // 生成该状态时造句的 total_length（= interpreted_length）
    size_t total_length = 0;
  };

  Poet(const Language* language,
       Config* config,
       Compare compare = CompareWeight);
  ~Poet();

  an<Sentence> MakeSentence(const WordGraph& graph,
                            size_t total_length,
                            const string& preceding_text,
                            an<Lattice>* lattice = nullptr);

  // 增量造句：graph 在上一次造句（lattice->total_length 为当时的
  // total_length）的 WordGraph 基础上追加了新桶（end 更大的词条组），
  // 复用上次的前向搜索状态，只计算新增位置，结果与全量重跑完全一致。
  // 失败返回 nullptr，调用方应回退全量 MakeSentence。
  an<Sentence> MakeSentenceIncremental(const WordGraph& graph,
                                       size_t total_length,
                                       const string& preceding_text,
                                       an<Lattice>& lattice);

  template <class TranslatorT>
  an<Translation> ContextualWeighted(an<Translation> translation,
                                     const string& input,
                                     size_t start,
                                     TranslatorT* translator) {
    if (!translator->contextual_suggestions() || !grammar_) {
      return translation;
    }
    auto preceding_text = translator->GetPrecedingText(start);
    if (preceding_text.empty()) {
      return translation;
    }
    return New<ContextualTranslation>(translation, input, preceding_text,
                                      grammar_.get());
  }

 private:
  template <class Strategy>
  struct TypedLattice;

  template <class Strategy>
  using StateMap = map<int, typename Strategy::State>;

  // 把 start_pos 的出边（end_pos >= min_end_pos）展开进 states。
  // rear_end_pos：is_rear 判定的终点位置；exclude_single_word：是否
  // 排除 0→rear_end_pos 的单词边。
  template <class Strategy>
  void ExpandEdges(StateMap<Strategy>& states,
                   size_t start_pos,
                   const map<int, DictEntryList>& edges,
                   size_t min_end_pos,
                   size_t rear_end_pos,
                   bool exclude_single_word,
                   const string& preceding_text);

  template <class Strategy>
  an<Sentence> ExtractBestSentence(const StateMap<Strategy>& states,
                                   size_t total_length) const;

  template <class Strategy>
  an<Sentence> MakeSentenceWithStrategy(const WordGraph& graph,
                                        size_t total_length,
                                        const string& preceding_text,
                                        an<Lattice>* lattice);

  template <class Strategy>
  an<Sentence> MakeSentenceIncrementalWithStrategy(
      TypedLattice<Strategy>& lattice,
      const WordGraph& graph,
      size_t total_length,
      const string& preceding_text);

  const Language* language_;
  the<Grammar> grammar_;
  Compare compare_;
};

}  // namespace rime

#endif  // RIME_POET_H_
