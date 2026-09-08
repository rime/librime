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
  // 將實體按鍵轉換爲宮保代碼
  char32_t ConvertToChordKey(int keycode) const;
  // 重置按鍵時序追蹤
  void ResetTracking();
  // 注入轉換後的並擊碼至輸入緩衝區
  void FlushChordKey(ChordKeyEvent key_event);
  // 常規按鍵時序與並擊邊界處理
  ProcessResult HandleChordKey(ChordKeyEvent key_event);
  // 重發原生空格 (組詞態選詞, 空閒態輸出真空格)
  void ReplayPendingSpace();
  // 顯示暫存空格標記
  void DisplayPendingSpace();
  // 清空暫存空格標記
  void ClearPendingSpace();
  // 將已完成的並擊和弦更新爲定界的標準化和弦
  void CanonicalizeCurrentChord();

  // 鍵位映射表: 物理鍵碼 -> 宮保代碼
  std::map<int, char32_t> key_map_;

  // 方案配置參數
  std::u32string initial_keys_;    // 方案聲母/首部鍵集
  std::u32string final_keys_;      // 方案韻母/尾部鍵集
  int chord_timeout_ms_ = 120;     // 異音節連打切分超時 (ms)
  int chord_duration_ms_ = 60;     // 同和弦雙手落鍵生理容差窗口 (ms)
  char delimiter_ = '\'';          // 隔音符號
  the<Projection> canonicalizer_;  // 並擊和弦標準化運算

  // 時序狀態追蹤
  ChordKeyEvent last_key_event_;
  ChordKeyEvent pending_space_;  // 空閒態/孤立空格暫存

  // 物理抬鍵追蹤狀態
  std::set<int> pressed_chord_keys_;  // 當前處於按下狀態的並擊鍵集合
  size_t current_chord_start_;        // 當前並擊和弦的編碼起始位置

  // 防遞歸標記
  bool is_replaying_ = false;
};

}  // namespace rime

#endif  // RIME_STREAMING_CHORD_PROCESSOR_H_
