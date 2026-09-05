//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2011-08-07 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_SCRIPT_TRANSLATOR_H_
#define RIME_SCRIPT_TRANSLATOR_H_

#include <rime/common.h>
#include <rime/translation.h>
#include <rime/translator.h>
#include <rime/algo/algebra.h>
#include <rime/gear/memory.h>
#include <rime/gear/poet.h>
#include <rime/gear/translator_commons.h>

namespace rime {

class Code;
class Corrector;
struct DictEntry;
class Dictionary;
struct SyllableGraph;
class UserDictionary;

// T9（九键）快速输入的增量 compose 缓存（每个 translator 实例一份）。
// 仅对「单段起点 + 编码串严格追加 + 纪元/词典版本未变」的查询生效，
// 其余情况自动回退全量路径；对全拼等多段输入不产生任何行为差异。
struct ScriptIncrementalState {
  bool enabled = false;            // 配置 translator/incremental_compose
  string input;                    // 上次参与合成的查询输入（段内编码串）
  size_t interpreted_length = 0;   // 上次音节图解释长度
  uint64_t composition_epoch = 0;  // 上次合成时的 Context 纪元
  uint64_t user_dict_tick = 0;     // 上次合成时的用户词典版本
  uint32_t dict_checksum = 0;      // prism 指纹，防词典重部署后误用
  uint32_t schema_checksum = 0;
  string preceding_text;           // 上次 GetPrecedingText(0) 结果
  WordGraph graph;                 // 桶缓存（旧桶复用，新桶追加）
  an<Poet::Lattice> lattice;       // Poet 前向搜索状态
  SyllableGraphCache syllable_cache;  // 音节图增量缓存（syllabifier 持有）
  bool has_graph = false;          // graph/lattice 与上述指纹配套有效
};

class ScriptTranslator : public Translator,
                         public Memory,
                         public TranslatorOptions {
 public:
  ScriptTranslator(const Ticket& ticket);

  virtual an<Translation> Query(const string& input,
                                const Segment& segment) override;
  virtual bool Memorize(const CommitEntry& commit_entry) override;
  virtual bool ProcessSegmentOnCommit(CommitEntry& commit_entry,
                                      const Segment& seg) override;

  string FormatPreedit(const string& preedit);
  string Spell(const Code& code);
  string GetPrecedingText(size_t start) const;
  bool UpdateElements(const CommitEntry& commit_entry);

  // ── T9 增量 compose（详见 ScriptIncrementalState）──
  ScriptIncrementalState* incremental_state() { return incremental_.get(); }
  // 守卫：单段起点 + 严格追加 + 纪元/词典版本未变；通过时输出旧解释长度
  bool TryBeginIncremental(const string& input,
                           UserDictionary* user_dict,
                           size_t* old_interpreted);
  // 一轮合成结束后更新缓存指纹；cacheable=false 时作废桶/lattice 缓存
  void CommitIncremental(const string& input,
                         size_t interpreted_length,
                         UserDictionary* user_dict,
                         const string& preceding_text,
                         bool cacheable);

  bool ConcatenatePhrases(CommitEntry& commit_entry,
                          const vector<an<Phrase>>& phrases);
  bool SaveCommitEntry(CommitEntry& commit_entry);

  // options
  int max_homophones() const { return max_homophones_; }
  int spelling_hints() const { return spelling_hints_; }
  bool always_show_comments() const { return always_show_comments_; }
  bool enable_word_completion() const { return enable_word_completion_; }
  int max_word_length() const { return max_word_length_; }
  int core_word_length() const;

 protected:
  int max_homophones_ = 1;
  int spelling_hints_ = 0;
  int max_word_length_ = 0;
  int core_word_length_ = 0;
  bool always_show_comments_ = false;
  bool enable_correction_ = false;
  bool enable_word_completion_ = false;
  bool incremental_enabled_ = true;
  the<Corrector> corrector_;
  the<Poet> poet_;
  the<ScriptIncrementalState> incremental_;
  vector<an<Phrase>> queue_;
};

}  // namespace rime

#endif  // RIME_SCRIPT_TRANSLATOR_H_
