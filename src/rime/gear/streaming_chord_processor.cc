#include "streaming_chord_processor.h"
#include <cctype>
#include <utf8.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/config.h>
#include <rime/algo/algebra.h>

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

  utf8::unchecked::utf8to32(initials.begin(), initials.end(),
                            std::back_inserter(initial_keys_));
  utf8::unchecked::utf8to32(finals.begin(), finals.end(),
                            std::back_inserter(final_keys_));

  // 讀取串流隔音符號：
  // 優先讀取 streaming_chord/delimiter (允許手動設爲 "'" 或空字串 "")
  // 若未專門配置，則自動向 speller/delimiter 索取第一個非空格字符 (通常是 ')
  string chord_delim;
  if (config->GetString("streaming_chord/delimiter", &chord_delim)) {
    delimiter_ = chord_delim;
  } else {
    string speller_delim;
    if (config->GetString("speller/delimiter", &speller_delim)) {
      auto pos = speller_delim.find_first_not_of(' ');
      if (pos != string::npos) {
        delimiter_ = string(1, speller_delim[pos]);  // 精確提取到可見的單引號 '
      }
    }
  }

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

      if (keycode != 0) {
        auto it_str = target_str.begin();
        char32_t target_ch = utf8::unchecked::next(it_str);
        key_map_[keycode] = target_ch;

        // 非字母的並擊鍵 (空格、分號等)，自動註冊爲雙功能鍵
        if (keycode >= 0x20 && keycode <= 0x7e && !std::isalpha(keycode)) {
          dual_role_keys_.insert(keycode);
        }
      }
    }
  }

  // 3. 額外配置補充 (可選，允許手動覆蓋或指定非 ASCII 功能鍵)
  if (an<ConfigList> dual_list =
          config->GetList("streaming_chord/dual_role_keys")) {
    for (size_t i = 0; i < dual_list->size(); ++i) {
      if (auto val = dual_list->GetValueAt(i)) {
        KeyEvent ke;
        if (ke.Parse(val->str()) && ke.keycode() != 0) {
          dual_role_keys_.insert(ke.keycode());
        }
      }
    }
  }

  // 4. 並擊和弦標準化運算
  if (an<ConfigList> rules = config->GetList("streaming_chord/canonicalize")) {
    the<Projection> canonicalizer(new Projection());
    if (canonicalizer->Load(rules)) {
      canonicalizer_ = std::move(canonicalizer);
    }
  }

  // 聲明方案使用並擊特性 (啓用鼠鬚管抬鍵轉發通道)
  Context* ctx = engine_->context();
  ctx->set_option("_chord_typing", true);
}

bool StreamingChordProcessor::IsInitial(char32_t key) const {
  return initial_keys_.find(key) != std::u32string::npos;
}

bool StreamingChordProcessor::IsFinal(char32_t key) const {
  return final_keys_.find(key) != std::u32string::npos;
}

bool StreamingChordProcessor::IsDualRoleKey(int keycode) const {
  // 必須「參與了並擊映射」且「登記在雙功能清單中」，二者缺一不可
  return key_map_.find(keycode) != key_map_.end() &&
         dual_role_keys_.find(keycode) != dual_role_keys_.end();
}

char32_t StreamingChordProcessor::ConvertToChordKey(int keycode) const {
  auto it = key_map_.find(keycode);
  if (it != key_map_.end()) {
    return it->second;
  }
  return static_cast<char32_t>(keycode);
}

void StreamingChordProcessor::ResetTracking() {
  last_key_event_ = {};
  pending_solo_key_ = {};
  pressed_chord_keys_.clear();
  current_chord_start_ = 0;
  is_chord_open_ = false;
}

void StreamingChordProcessor::FlushChordKey(ChordKeyEvent key_event) {
  string input_str;
  utf8::unchecked::append(key_event.key, std::back_inserter(input_str));
  Context* context = engine_->context();

  // 若當前和弦尚未開啟（前一個已閉合，或是全新輸入），錨定新起點
  if (!is_chord_open_) {
    current_chord_start_ = context->input().length();
    is_chord_open_ = true;
  }

  context->PushInput(input_str);
  last_key_event_ = key_event;
  pressed_chord_keys_.insert(key_event.keycode);
}

ProcessResult StreamingChordProcessor::HandleChordKey(ChordKeyEvent key_event) {
  bool is_initial = IsInitial(key_event.key);
  bool is_final = IsFinal(key_event.key);

  // 非並擊字母交給後續組件處理
  if (!is_initial && !is_final) {
    return kNoop;
  }

  // 只有當「前一個和弦依然敞開」時，時序邊界才代表「音節切分」
  if (is_chord_open_ && last_key_event_) {
    auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                       key_event.time - last_key_event_.time)
                       .count();

    // 條件一 超時邊界: 連打思考停頓 / 同手同區連打超時切分
    bool is_timeout_boundary = (delta_t > chord_timeout_ms_);

    // 條件二 手系逆轉邊界: 右手韻母 -> 左手聲母 且超出微時差容差窗口
    bool is_phase_inversion = IsFinal(last_key_event_.key) && is_initial &&
                              (delta_t > chord_duration_ms_);

    // 命中邊界：代表無抬鍵平臺（Emacs）連打時前一音節結束
    if (is_timeout_boundary || is_phase_inversion) {
      // 爲無抬鍵平臺注入可見隔音符 (如 "'")，輔助 Translator 分詞
      if (!delimiter_.empty()) {
        Context* context = engine_->context();
        context->PushInput(delimiter_);
      }

      // 宣告前一和弦在流式時序上已被強制截斷，復位狀態
      pressed_chord_keys_.clear();
      is_chord_open_ = false;
      current_chord_start_ = 0;
    }
  }

  FlushChordKey(key_event);
  return kAccepted;
}

void StreamingChordProcessor::ReplayPendingKey() {
  if (!pending_solo_key_)
    return;
  int keycode = pending_solo_key_.keycode;
  pending_solo_key_ = {};
  ClearPendingPrompt();

  Context* context = engine_->context();
  is_replaying_ = true;

  // 將原始按鍵重新注入引擎流水線
  KeyEvent raw_key(keycode, 0);

  // 1. 組詞態下：交給 selector 選詞或 punctuator 上屏標點
  if (context->IsComposing() && !context->input().empty()) {
    engine_->ProcessKey(raw_key);
  } else {
    // 2. 空閒態下：若下游處理器未截獲，只要屬於可列印 ASCII
    // 範圍，直接上屏原生字元
    if (!engine_->ProcessKey(raw_key)) {
      if (keycode >= 0x20 && keycode <= 0x7e) {
        engine_->CommitText(string(1, static_cast<char>(keycode)));
      }
    }
  }

  is_replaying_ = false;
}

static const string kSpaceSymbol = "␣";

string StreamingChordProcessor::GetPendingPrompt(int keycode) const {
  if (keycode == XK_space) {
    return "␣";  // 空格使用專用標記符號
  }
  // 可列印 ASCII 標點鍵包裹方括號，明確提示「此鍵處於待判定狀態」
  if (keycode >= 0x20 && keycode <= 0x7e) {
    return "[" + string(1, static_cast<char>(keycode)) + "]";
  }
  return "·";
}

void StreamingChordProcessor::DisplayPendingPrompt(int keycode) {
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();

  // 若當前處於空閒態，構造一個長度爲 0 的 phony 切片承載提示符
  if (comp.empty()) {
    Segment placeholder(0, ctx->input().length());
    placeholder.tags.insert("phony");
    ctx->composition().AddSegment(placeholder);
  }

  auto& last_segment = comp.back();
  last_segment.tags.insert("chord_prompt");
  last_segment.prompt = GetPendingPrompt(keycode);
}

void StreamingChordProcessor::ClearPendingPrompt() {
  if (!engine_)
    return;
  Context* ctx = engine_->context();
  Composition& comp = ctx->composition();
  if (comp.empty())
    return;

  auto& last_segment = comp.back();
  if (comp.size() == 1 && last_segment.HasTag("phony")) {
    // 若只有虛擬切片，徹底清空 context，使 emacs-rime 關閉浮動框
    ctx->Clear();
  } else if (last_segment.HasTag("chord_prompt")) {
    // 若是在正常組詞中途暫存，僅抹去提示符文字與標籤
    last_segment.prompt.clear();
    last_segment.tags.erase("chord_prompt");
  }
}

void StreamingChordProcessor::CanonicalizeCurrentChord() {
  Context* context = engine_->context();
  auto chord = context->input().substr(current_chord_start_);
  if (chord.empty())
    return;
  // 將生和弦交由 canonicalizer 替換爲標準閉包（如 [ZFURO] 或拼音）
  if (canonicalizer_) {
    context->PopInput(chord.length());
    canonicalizer_->Apply(&chord);
    context->PushInput(chord);
  }
}

ProcessResult StreamingChordProcessor::ProcessKeyEvent(
    const KeyEvent& key_event) {
  if (is_replaying_) {
    return kNoop;
  }

  Context* context = engine_->context();
  int keycode = key_event.keycode();

  // 1. 抬鍵處理 (有 KeyUp 事件的 GUI 平臺)
  if (key_event.release()) {
    // 孤立雙功能鍵抬起 (單擊空格出空格/選詞, 單擊分號出分號), 手指一抬即時結算
    if (pending_solo_key_ && keycode == pending_solo_key_.keycode) {
      ReplayPendingKey();
      ResetTracking();
      return kAccepted;
    }

    auto it = pressed_chord_keys_.find(keycode);
    if (it != pressed_chord_keys_.end()) {
      pressed_chord_keys_.erase(it);
      // 當所有按下的並擊鍵盡數釋放，和弦閉合
      if (pressed_chord_keys_.empty()) {
        if (context->IsComposing()) {
          CanonicalizeCurrentChord();
        }
        is_chord_open_ = false;  // 抬手，自然關閉
        current_chord_start_ = 0;
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

  // 2. 處理雙功能鍵 (空格、分號等)
  if (IsDualRoleKey(keycode)) {
    ChordKeyEvent solo_key{keycode, chord_key, now};

    // 情況 A: 前次敲擊的同一個雙功能鍵尚在暫存中 -> 檢驗是否構成連擊
    if (pending_solo_key_ && pending_solo_key_.keycode == keycode) {
      // 連擊雙功能鍵: 結算第 1 記, 吞掉當前第 2 記
      ReplayPendingKey();
      ResetTracking();
      return kAccepted;
    }

    // 情況 B: 檢驗是否與前序按鍵構成同和弦並擊 (如聲母後落鍵打 da 或帶調音節)
    if (last_key_event_) {
      auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_key_event_.time)
                         .count();
      if (delta_t <= chord_duration_ms_) {
        // 判定爲同音節並擊成分 (A 或 Y), 立即推入緩衝區
        return HandleChordKey(solo_key);
      }
    }

    // 情況 C: 孤立雙功能鍵落鍵, 暫存等待後續鍵裁決 (支持抬鍵即上屏)
    // 若此前已有其他不同的暫存鍵, 先將舊鍵重發
    if (pending_solo_key_) {
      ReplayPendingKey();
    }
    pending_solo_key_ = solo_key;
    DisplayPendingPrompt(keycode);
    return kAccepted;
  }

  // 3. 結算孤立暫存雙功能鍵 (後續按鍵到達)
  if (pending_solo_key_) {
    auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - pending_solo_key_.time)
                       .count();

    bool is_chord_key = IsInitial(chord_key) || IsFinal(chord_key);

    if (is_chord_key && delta_t <= chord_duration_ms_) {
      // 雙功能鍵超前落鍵並擊 (如 ; -> J 打帶調音節, 或 Space -> D 打 da)
      ChordKeyEvent saved_key = pending_solo_key_;
      pending_solo_key_ = {};
      ClearPendingPrompt();
      FlushChordKey(saved_key);
    } else {
      // 超時或按下非並擊鍵: 確證爲獨立單擊, 重發結算
      ReplayPendingKey();
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
