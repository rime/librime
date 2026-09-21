#ifndef RIME_STREAMING_CHORD_PROCESSOR_H_
#define RIME_STREAMING_CHORD_PROCESSOR_H_

#include <chrono>
#include <map>
#include <set>
#include <string>
#include <rime/common.h>
#include <rime/component.h>
#include <rime/processor.h>

namespace rime {

class Projection;

struct ChordKeyEvent {
  int keycode = 0;
  char32_t key = 0;
  std::chrono::steady_clock::time_point time{};

  operator bool() const { return keycode != 0; }
};

class StreamingChordProcessor : public Processor {
 public:
  StreamingChordProcessor(const Ticket& ticket);

  ProcessResult ProcessKeyEvent(const KeyEvent& key_event) override;

 protected:
  bool IsInitial(char32_t key) const;
  bool IsFinal(char32_t key) const;
  bool IsMappedKey(int keycode) const;
  bool IsDualRoleKey(int keycode) const;
  // 將實體按鍵轉換爲宮保代碼
  char32_t ConvertToChordKey(int keycode) const;
  // 重置按鍵時序追蹤
  void ResetTracking();
  // 注入轉換後的並擊碼至輸入緩衝區
  void FlushChordKey(ChordKeyEvent key_event);
  // 常規按鍵時序與並擊邊界處理
  ProcessResult HandleChordKey(ChordKeyEvent key_event);
  // 重發原生雙功能鍵 (組詞態選詞/符號上屏, 空閒態輸出真字符)
  void ReplayPendingKey();
  // 暫存按鍵標記，待判定的雙功能鍵顯示爲 [落鍵字符]，空格顯示 ␣
  string GetPendingPrompt(int keycode) const;
  // 顯示暫存按鍵標記
  void DisplayPendingPrompt(int keycode);
  // 清空暫存按鍵標記
  void ClearPendingPrompt();
  // 將已完成的並擊和弦更新爲定界的標準化和弦
  void CanonicalizeCurrentChord();
  // 匹配並剔除和弦尾部的動作後綴標籤，返回待執行的按鍵事件
  KeyEvent PopAction(string* chord);

  // 鍵位映射表: 物理鍵碼 -> 宮保代碼
  map<int, char32_t> key_map_;

  // 雙功能鍵清單 (單擊回放原始鍵, 並擊充當和弦碼)
  set<int> dual_role_keys_;

  // 方案配置參數
  std::u32string initial_keys_;    // 方案聲母/首部鍵集
  std::u32string final_keys_;      // 方案韻母/尾部鍵集
  int chord_timeout_ms_ = 120;     // 異音節連打切分超時 (ms)
  int chord_duration_ms_ = 60;     // 同和弦雙手落鍵生理容差窗口 (ms)
  string delimiter_;               // 串流隔音符號
  the<Projection> canonicalizer_;  // 並擊和弦標準化運算
  vector<pair<string, KeyEvent>> action_suffixes_;  // 動作和弦後綴

  // 時序狀態追蹤
  ChordKeyEvent last_key_event_;
  ChordKeyEvent pending_solo_key_;  // 空閒態/孤立雙功能鍵暫存 (空格/分號等)

  // 物理抬鍵追蹤狀態
  set<int> pressed_chord_keys_;     // 當前處於按下狀態的並擊鍵集合
  size_t current_chord_start_ = 0;  // 當前並擊和弦的編碼起始位置
  bool is_chord_open_ = false;      // 當前是否處於未封閉的生和弦中

  // 防遞歸標記
  bool is_replaying_ = false;
};

}  // namespace rime

#endif  // RIME_STREAMING_CHORD_PROCESSOR_H_
