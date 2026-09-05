#include "streaming_chord_processor.h"
#include <cctype>
#include <utf8.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/config.h>

// 一張機, 免除異步定時器:
// 不開闢後台定時線程, 僅依靠新落鍵觸發舊音節前置結算.
// 二張機, 雙手並發微時差保護:
// 若右手韻母比左手聲母提早觸底 (如 A → S, Δt = 15ms < 35ms), 手系逆轉判定爲否,
// 不加隔音符號, 交給 canonicalizer 規範化重排爲 SA.
// 三張機, 緩衝區同步重置:
// 退格或提交導致輸入爲空時最後按鍵自動歸零, 防止退格後重新輸入時誤加分隔符.
// 四張機, 拇指空格雙模分流:
// 孤立空格落鍵暫存, 抬鍵即時結算選詞/真空格; 無抬鍵時流式雙擊亦可兜底閉環.
// 五張機, 雙全釋放見真章:
// 抬鍵盡釋則音節斷, 物理爲憑, 與流式時序相輔相成, 毫秒無差.

namespace rime {

StreamingChordProcessor::StreamingChordProcessor(const Ticket& ticket)
    : Processor(ticket) {
  Config* config = engine_->schema()->config();
  if (!config)
    return;

  // 1. 讀取並擊分區定義與時序閾值
  string initials = "SCZHLFGDBKTP";
  string finals = "IUVANREO";
  config->GetString("streaming_chord/initial_keys", &initials);
  config->GetString("streaming_chord/final_keys", &finals);
  config->GetInt("streaming_chord/chord_timeout_ms", &chord_timeout_ms_);
  config->GetInt("streaming_chord/chord_duration_ms", &chord_duration_ms_);

  string delim = "'";
  if (config->GetString("speller/delimiter", &delim) && !delim.empty()) {
    delimiter_ = delim[0];
  }

  utf8::unchecked::utf8to32(initials.begin(), initials.end(),
                            std::back_inserter(initial_keys_));
  utf8::unchecked::utf8to32(finals.begin(), finals.end(),
                            std::back_inserter(final_keys_));

  // 2. 載入鍵位映射配置 (物理鍵碼 -> 宮保字母)
  if (an<ConfigMap> key_map = config->GetMap("streaming_chord/key_map")) {
    for (auto it = key_map->begin(); it != key_map->end(); ++it) {
      string key_str = it->first;
      auto val = As<ConfigValue>(it->second);
      if (!val)
        continue;
      string target_str = val->str();
      if (key_str.empty() || target_str.empty())
        continue;

      int keycode = 0;
      KeyEvent ke;
      if (ke.Parse(key_str)) {
        keycode = ke.keycode();
      }

      auto it_str = target_str.begin();
      char32_t target_ch = utf8::unchecked::next(it_str);
      if (keycode != 0) {
        key_map_[keycode] = target_ch;
      }
    }
  }

  // 3. 聲明方案使用並擊特性 (啓用鼠鬚管抬鍵轉發通道)
  Context* ctx = engine_->context();
  ctx->set_option("_chord_typing", true);
}

bool StreamingChordProcessor::IsInitial(char32_t key) const {
  return initial_keys_.find(key) != std::u32string::npos;
}

bool StreamingChordProcessor::IsFinal(char32_t key) const {
  return final_keys_.find(key) != std::u32string::npos;
}

char32_t StreamingChordProcessor::ConvertToChordKey(int keycode) const {
  auto it = key_map_.find(keycode);
  if (it != key_map_.end()) {
    return it->second;
  } else {
    return static_cast<char32_t>(keycode);
  }
}

void StreamingChordProcessor::ResetTracking() {
  last_key_event_ = {};
  pending_space_ = {};
  pressed_chord_keys_.clear();
  chord_released_ = false;
}

void StreamingChordProcessor::FlushChordKey(ChordKeyEvent key_event) {
  string input_str;
  utf8::unchecked::append(key_event.key, std::back_inserter(input_str));
  Context* context = engine_->context();
  context->PushInput(input_str);
  last_key_event_ = key_event;
  pressed_chord_keys_.insert(key_event.keycode);
  chord_released_ = false;  // 新鍵落底, 和弦重回握持狀態
}

ProcessResult StreamingChordProcessor::HandleChordKey(ChordKeyEvent key_event) {
  bool is_initial = IsInitial(key_event.key);
  bool is_final = IsFinal(key_event.key);

  // 非並擊字母交給後續組件處理
  if (!is_initial && !is_final) {
    return kNoop;
  }

  // 時序、相位與物理邊界三重判定
  if (last_key_event_) {
    auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                       key_event.time - last_key_event_.time)
                       .count();

    // 條件一 (物理硬邊界): 上一音節所有實體鍵已完全抬起 (有抬鍵環境專享)
    bool is_chord_release = chord_released_;

    // 條件二 (超時邊界): 連打思考停頓 / 同手同區連打超時切分
    bool is_timeout_boundary = (delta_t > chord_timeout_ms_);

    // 條件三 (手系逆轉邊界): 右手韻母 -> 左手聲母 且超出微時差容差窗口
    bool is_phase_inversion = IsFinal(last_key_event_.key) && is_initial &&
                              (delta_t > chord_duration_ms_);

    if (is_chord_release || is_timeout_boundary || is_phase_inversion) {
      Context* context = engine_->context();
      context->PushInput(delimiter_);
      chord_released_ = false;
      // 爲無抬鍵環境做防禦性清理, 避免集合只進不出
      pressed_chord_keys_.clear();
    }
  }

  FlushChordKey(key_event);
  return kAccepted;
}

void StreamingChordProcessor::ReplayPendingSpace() {
  if (!pending_space_)
    return;
  pending_space_ = {};
  ClearPendingSpace();

  Context* context = engine_->context();
  is_replaying_ = true;

  // 若處於組詞態, 重發空格交給 selector 確認第 1 候選上屏;
  // 空閒態則向屏幕提交原生空格
  if (context->IsComposing() && !context->input().empty()) {
    engine_->ProcessKey(KeyEvent(XK_space, 0));
  } else {
    if (!engine_->ProcessKey(KeyEvent(XK_space, 0))) {
      engine_->CommitText(" ");
    }
  }

  is_replaying_ = false;
}

static const string kSpaceSymbol = "␣";

void StreamingChordProcessor::DisplayPendingSpace() {
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();
  if (comp.empty()) {
    Segment placeholder(0, ctx->input().length());
    placeholder.tags.insert("phony");
    ctx->composition().AddSegment(placeholder);
  }
  auto& last_segment = comp.back();
  last_segment.tags.insert("chord_prompt");
  last_segment.prompt = kSpaceSymbol;
}

void StreamingChordProcessor::ClearPendingSpace() {
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();
  if (comp.empty())
    return;
  auto& last_segment = comp.back();
  if (comp.size() == 1 && last_segment.HasTag("phony")) {
    ctx->Clear();
  } else if (last_segment.HasTag("chord_prompt")) {
    last_segment.prompt.clear();
    last_segment.tags.erase("chord_prompt");
  }
}

ProcessResult StreamingChordProcessor::ProcessKeyEvent(
    const KeyEvent& key_event) {
  if (is_replaying_) {
    return kNoop;
  }

  Context* context = engine_->context();

  int keycode = key_event.keycode();

  // 1. 抬鍵處理 (純被動監聽, 絕不阻斷)
  if (key_event.release()) {
    // 孤立空格抬起, 確證爲單擊選詞/出真空格, 手指一抬即時結算
    if (keycode == XK_space && pending_space_) {
      ReplayPendingSpace();
      ResetTracking();
      return kAccepted;
    }

    auto it = pressed_chord_keys_.find(keycode);
    if (it != pressed_chord_keys_.end()) {
      pressed_chord_keys_.erase(it);
      // 當所有握持鍵全數釋放, 且正在組詞, 確證上一物理和弦結束
      if (pressed_chord_keys_.empty() && context->IsComposing()) {
        chord_released_ = true;
      }
      return kAccepted;
    }
    return kNoop;
  }

  // 忽略功能組合修飾鍵 (Ctrl/Alt/Super)
  if (key_event.ctrl() || key_event.alt() || key_event.super()) {
    return kNoop;
  }

  char32_t chord_key = ConvertToChordKey(keycode);
  auto now = std::chrono::steady_clock::now();

  // 2. 處理空格鍵
  if (keycode == XK_space) {
    ChordKeyEvent space_key{keycode, chord_key, now};

    // 情況 A: 前次敲擊的空格尚在暫存中 -> 檢驗是否構成雙擊
    if (pending_space_) {
      // 雙擊空格: 結算第 1 記空格 (組詞態選詞/空閒態出真空格),
      // 吞掉當前第 2 記空格
      ReplayPendingSpace();
      ResetTracking();
      return kAccepted;
    }

    // 情況 B: 檢驗是否與前序按鍵構成同和弦並擊 (如聲母後落鍵打 da)
    if (last_key_event_) {
      auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_key_event_.time)
                         .count();
      if (delta_t <= chord_duration_ms_) {
        // 判定爲同音節並擊韻母 A, 立即推入緩衝區
        return HandleChordKey(space_key);
      }
    }

    // 情況 C: 孤立空格落鍵, 暫存等待後續鍵裁決 (支持抬鍵即上屏)
    pending_space_ = space_key;
    DisplayPendingSpace();
    return kAccepted;
  }

  // 3. 結算孤立暫存空格 (後續按鍵到達)
  if (pending_space_) {
    auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - pending_space_.time)
                       .count();

    bool is_schema_chord_key = IsInitial(chord_key) || IsFinal(chord_key);

    if (is_schema_chord_key && delta_t <= chord_duration_ms_) {
      // 拇指超前落鍵並擊 (如 Space -> D 並擊 da): 結算暫存空格爲 A
      ChordKeyEvent saved_space = pending_space_;
      pending_space_ = {};
      ClearPendingSpace();
      FlushChordKey(saved_space);
    } else {
      // 超時或按下非並擊鍵: 先前空格確認爲獨立空格, 重發結算
      ReplayPendingSpace();
    }
  }

  // 4. 編輯與確認控制鍵放行
  if (keycode == XK_BackSpace || keycode == XK_Return || keycode == XK_Escape) {
    ResetTracking();
    return kNoop;
  }

  if (!context->IsComposing() || context->input().empty()) {
    ResetTracking();
  }

  // 5. 常規並擊按鍵處理
  return HandleChordKey(ChordKeyEvent{keycode, chord_key, now});
}

}  // namespace rime
