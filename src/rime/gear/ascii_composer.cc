#include <rime/common.h>
#include <rime/composition.h>
#include <rime/config.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/key_event.h>
#include <rime/key_table.h>
#include <rime/schema.h>
#include <rime/gear/ascii_composer.h>

namespace rime {

static struct AsciiModeSwitchStyleDefinition {
  const char* repr;
  AsciiModeSwitchStyle style;
} ascii_mode_switch_styles[] = {{"inline_ascii", kAsciiModeSwitchInline},
                                {"commit_text", kAsciiModeSwitchCommitText},
                                {"commit_code", kAsciiModeSwitchCommitCode},
                                {"clear", kAsciiModeSwitchClear},
                                {"set_ascii_mode", kAsciiModeSet},
                                {"unset_ascii_mode", kAsciiModeUnset},
                                {NULL, kAsciiModeSwitchNoop}};

static bool load_bindings(const an<ConfigMap>& src,
                          AsciiModeSwitchKeyBindings* dest) {
  if (!src || !dest)
    return false;
  for (auto it = src->begin(); it != src->end(); ++it) {
    auto value = As<ConfigValue>(it->second);
    if (!value)
      continue;
    auto* p = ascii_mode_switch_styles;
    while (p->repr && p->repr != value->str())
      ++p;
    if (p->style == kAsciiModeSwitchNoop)
      continue;
    KeyEvent ke;
    if (!ke.Parse(it->first) || ke.modifier() != 0) {
      LOG(WARNING) << "invalid ascii mode switch key: " << it->first;
      continue;
    }
    // save binding
    (*dest)[ke.keycode()] = p->style;
  }
  return true;
}

static AsciiComposer::ActionDef action_definitions[] = {
    {"commit_raw_input", &AsciiComposer::CommitRawInput},
    {"commit_raw_input_with_space", &AsciiComposer::CommitRawInputWithSpace},
    AsciiComposer::kActionNoop,
};

AsciiComposer::AsciiComposer(const Ticket& ticket)
    : Processor(ticket),
      KeyBindingProcessor<AsciiComposer>(action_definitions) {
  LoadConfig(ticket.schema);

  if (Context* ctx = engine_->context()) {
    update_connection_ = ctx->update_notifier().connect(
        [this](Context* ctx) { OnContextUpdate(ctx); });
    commit_connection_ =
        ctx->commit_notifier().connect([this](Context* ctx) { OnCommit(ctx); });
  }
}

AsciiComposer::~AsciiComposer() {
  update_connection_.disconnect();
  commit_connection_.disconnect();
}

static inline bool IsKeypadDigitOrDecimal(int ch) {
  return (ch >= XK_KP_0 && ch <= XK_KP_9) || ch == XK_KP_Decimal;
}

static inline char KeypadToChar(int ch) {
  return ch == XK_KP_Decimal ? '.' : static_cast<char>('0' + (ch - XK_KP_0));
}

void AsciiComposer::ResetModifierState() {
  pending_toggle_key_.reset();
}

void AsciiComposer::CommitAndReset(const string& text) {
  Context* ctx = engine_->context();
  is_inline_ascii_ = false;

  engine_->CommitText(text);
  ctx->Clear();
  raw_keys_.clear();
  cached_input_.clear();
  cached_raw_keys_.clear();
  ResetModifierState();
  if (ctx->get_option("ascii_mode")) {
    ctx->set_option("ascii_mode", false);
  }
}

void AsciiComposer::EnterInlineAsciiWithChar(char ch) {
  Context* ctx = engine_->context();

  cached_input_ = ctx->input();
  cached_raw_keys_ = raw_keys_;
  raw_keys_.push_back(ch);

  // 抹除中文未確認候選菜單與和弦輸入，讓西文即時回顯
  ctx->ClearNonConfirmedComposition();

  ctx->set_input(raw_keys_);
  ctx->set_option("ascii_mode", true);
  is_inline_ascii_ = true;
}

ProcessResult AsciiComposer::ProcessKeyEvent(const KeyEvent& key_event) {
  DLOG(INFO) << "AsciiComposer: key = " << key_event.repr()
             << ", composing = " << engine_->context()->IsComposing()
             << ", raw_keys = " << raw_keys_;

  int ch = key_event.keycode();
  Context* ctx = engine_->context();
  bool ascii_mode = ctx->get_option("ascii_mode");

  // 1. 修飾鍵釋放與單擊切換判定（Shift / Ctrl / Alt / Super）
  bool is_shift = (ch == XK_Shift_L || ch == XK_Shift_R);
  bool is_ctrl = (ch == XK_Control_L || ch == XK_Control_R);
  bool is_alt = (ch == XK_Alt_L || ch == XK_Alt_R);
  bool is_super = (ch == XK_Super_L || ch == XK_Super_R);

  if (key_event.release()) {
    if (is_shift || is_ctrl || is_alt || is_super) {
      if (pending_toggle_key_) {
        auto now = std::chrono::steady_clock::now();
        if (*pending_toggle_key_ == ch && now < toggle_expired_) {
          ToggleAsciiModeWithKey(ch);
        }
        ResetModifierState();
        return kNoop;
      }
    }
    return kNoop;
  }

  // 2. 多修飾鍵互斥保護
  if (key_event.shift() + key_event.ctrl() + key_event.alt() +
          key_event.super() >
      1) {
    ResetModifierState();
    return kNoop;
  }

  // 3. CapsLock 處理
  if (caps_lock_switch_style_ != kAsciiModeSwitchNoop) {
    ProcessResult result = ProcessCapsLock(key_event);
    if (result != kNoop)
      return result;
  }

  // 4. 獨立修飾鍵按下（記錄單擊計時）
  if (ch == XK_Eisu_toggle) {
    ResetModifierState();
    ToggleAsciiModeWithKey(ch);
    return kAccepted;
  }
  if (is_shift || is_ctrl || is_alt || is_super) {
    if (!pending_toggle_key_) {
      pending_toggle_key_ = ch;
      toggle_expired_ =
          std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    } else if (*pending_toggle_key_ != ch) {
      ResetModifierState();
    }
    return kNoop;
  }

  // 任何常規實體按鍵按下，立刻解除單擊修飾鍵的武裝
  ResetModifierState();

  // 5. 自定義按鍵綁定：有配置則響應，未配置則流暢放行
  auto action_result = KeyBindingProcessor::ProcessKeyEvent(key_event, ctx);
  if (action_result != kNoop) {
    return action_result;
  }

  // 未被 bindings 截留的常規快捷鍵（Ctrl / Alt / Super）放行給操作系統
  if (key_event.ctrl() || key_event.alt() || key_event.super()) {
    return kNoop;
  }

  // 6. 輸入狀態下攔截小鍵盤數字與小數點（受開關保護）
  if (inline_keypad_ && ctx->IsComposing() && IsKeypadDigitOrDecimal(ch)) {
    char digit = KeypadToChar(ch);
    if (!ascii_mode) {
      EnterInlineAsciiWithChar(digit);
    } else {
      raw_keys_.push_back(digit);
      ctx->PushInput(digit);
    }
    return kAccepted;
  }

  // 7. 臨時西文模式結算行爲（空格或回車確認上屏，秒回中文）
  if (ascii_mode && ctx->IsComposing()) {
    if (ch == XK_space && key_event.modifier() == 0) {
      CommitAndReset(ctx->input() + " ");
      return kAccepted;
    }
    if ((ch == XK_Return || ch == XK_KP_Enter) && key_event.modifier() == 0) {
      CommitAndReset(ctx->input());
      return kAccepted;
    }
  }

  // 8. 中文態下遇大寫字母：切換爲臨時西文（受開關保護）
  // 若當前不在輸入中，且賬本末尾不是字母（如單敲的空格已獨立上屏），出清歷史殘賬
  if (!ctx->IsComposing() && !raw_keys_.empty() && !isalpha(raw_keys_.back())) {
    raw_keys_.clear();
    cached_input_.clear();
    cached_raw_keys_.clear();
  }

  auto starts_with_alpha = [](const string& str) {
    return str.empty() || isalpha(str.front());
  };

  if (inline_uppercase_ && !ascii_mode && (ch >= 'A' && ch <= 'Z') &&
      starts_with_alpha(raw_keys_)) {
    EnterInlineAsciiWithChar(static_cast<char>(ch));
    return kAccepted;
  }

  // 9. 全域純西文態且無輸入：極速直通操作系統
  if (ascii_mode && !ctx->IsComposing()) {
    return kRejected;
  }

  // 10. 物理按鍵記賬與編輯流
  if (ch >= 0x20 && ch <= 0x7e) {
    // 讓所有實體按鍵如實入棧
    raw_keys_.push_back(static_cast<char>(ch));

    // 若當前處於臨時西文模式且正在輸入：主動推入 context 顯示
    if (ascii_mode && ctx->IsComposing()) {
      ctx->PushInput(ch);
      return kAccepted;
    }
    return kNoop;  // 中文態放行給下游和弦/拼音處理器
  }

  // 11. 退格鍵同步出棧
  if (ch == XK_BackSpace) {
    if (!raw_keys_.empty()) {
      raw_keys_.pop_back();
    }
    cached_input_.clear();
    cached_raw_keys_.clear();
    return kNoop;
  }

  // 12. Escape 放棄輸入
  if (ch == XK_Escape) {
    raw_keys_.clear();
    cached_input_.clear();
    cached_raw_keys_.clear();
    return kNoop;
  }

  return kNoop;
}

ProcessResult AsciiComposer::ProcessCapsLock(const KeyEvent& key_event) {
  int ch = key_event.keycode();
  if (ch == XK_Caps_Lock) {
    if (!key_event.release()) {
      pending_toggle_key_.reset();
      // temporarily disable good-old (uppercase) Caps Lock as mode switch key
      // in case the user switched to ascii mode with other keys, eg. with Shift
      if (good_old_caps_lock_ && !toggle_with_caps_) {
        Context* ctx = engine_->context();
        bool ascii_mode = ctx->get_option("ascii_mode");
        if (ascii_mode) {
          return kRejected;
        }
      }
      toggle_with_caps_ = !key_event.caps();
      // NOTE: for Linux, Caps Lock modifier is clear when we are about to
      // turn it on; for Windows it is the opposite:
      // Caps Lock modifier has been set before we process VK_CAPITAL.
      // here we assume IBus' behavior and invert caps with ! operation.
      SwitchAsciiMode(!key_event.caps(), caps_lock_switch_style_);
      // When good_old_caps_lock is enabled, allow the key event to pass
      // through the input method to toggle the Caps Lock state.
      return good_old_caps_lock_ ? kRejected : kAccepted;
    } else {
      return kRejected;
    }
  }
  if (key_event.caps()) {
    if (!good_old_caps_lock_ && !key_event.release() && !key_event.ctrl() &&
        !key_event.alt() && !key_event.super() && isascii(ch) && isalpha(ch)) {
      // output ascii characters ignoring Caps Lock
      if (islower(ch))
        ch = toupper(ch);
      else if (isupper(ch))
        ch = tolower(ch);
      engine_->CommitText(string(1, ch));
      return kAccepted;
    } else {
      return kRejected;
    }
  }
  return kNoop;
}

bool AsciiComposer::CommitRawInput(Context* ctx) {
  if (raw_keys_.empty() && !ctx->IsComposing()) {
    return false;
  }
  CommitAndReset(raw_keys_.empty() ? ctx->input() : raw_keys_);
  return true;
}

bool AsciiComposer::CommitRawInputWithSpace(Context* ctx) {
  if (raw_keys_.empty() && !ctx->IsComposing()) {
    return false;
  }
  CommitAndReset((raw_keys_.empty() ? ctx->input() : raw_keys_) + " ");
  return true;
}

void AsciiComposer::LoadConfig(Schema* schema) {
  bindings_.clear();
  caps_lock_switch_style_ = kAsciiModeSwitchNoop;
  good_old_caps_lock_ = false;
  inline_uppercase_ = false;
  inline_keypad_ = false;

  if (!schema)
    return;
  Config* config = schema->config();
  the<Config> preset(Config::Require("config")->Create("default"));

  auto get_config_bool_fallback_default = [&](const string& key,
                                              bool* value) -> bool {
    return config->GetBool(key, value) ||
           (preset && preset->GetBool(key, value));
  };
  get_config_bool_fallback_default("ascii_composer/good_old_caps_lock",
                                   &good_old_caps_lock_);
  get_config_bool_fallback_default("ascii_composer/inline_uppercase",
                                   &inline_uppercase_);
  get_config_bool_fallback_default("ascii_composer/inline_keypad",
                                   &inline_keypad_);

  // 載入狀態切換鍵設定
  if (load_bindings(config->GetMap("ascii_composer/switch_key"), &bindings_) ||
      (preset && load_bindings(preset->GetMap("ascii_composer/switch_key"),
                               &bindings_))) {
    // 驗證切換動作
    auto it = bindings_.find(XK_Caps_Lock);
    if (it != bindings_.end()) {
      caps_lock_switch_style_ = it->second;
      if (caps_lock_switch_style_ == kAsciiModeSwitchInline ||
          caps_lock_switch_style_ == kAsciiModeSet ||
          caps_lock_switch_style_ == kAsciiModeUnset) {
        // can't do that
        caps_lock_switch_style_ = kAsciiModeSwitchClear;
      }
    }
  } else {
    LOG(ERROR) << "Missing ascii bindings.";
  }

  // 載入上屏動作按鍵綁定映射
  if (config->GetMap("ascii_composer/bindings")) {
    KeyBindingProcessor::LoadConfig(config, "ascii_composer");
  } else if (preset && preset->GetMap("ascii_composer/bindings")) {
    KeyBindingProcessor::LoadConfig(preset.get(), "ascii_composer");
  }
}

bool AsciiComposer::ToggleAsciiModeWithKey(int key_code) {
  auto it = bindings_.find(key_code);
  if (it == bindings_.end())
    return false;
  AsciiModeSwitchStyle style = it->second;
  Context* ctx = engine_->context();
  bool old_mode = ctx->get_option("ascii_mode");
  bool new_mode = (style == kAsciiModeSet)     ? true
                  : (style == kAsciiModeUnset) ? false
                                               : !old_mode;
  if (old_mode == new_mode) {
    return false;
  }
  SwitchAsciiMode(new_mode, style);
  toggle_with_caps_ = (key_code == XK_Caps_Lock);
  return true;
}

void AsciiComposer::SwitchAsciiMode(bool ascii_mode,
                                    AsciiModeSwitchStyle style) {
  Context* ctx = engine_->context();

  // 若當前不在輸入中，直截了當地切換選項並退出
  if (!ctx->IsComposing()) {
    ctx->set_option("ascii_mode", ascii_mode);
    return;
  }

  is_inline_ascii_ = false;

  switch (style) {
    case kAsciiModeSwitchInline:
      TransitInlineAscii(ctx, ascii_mode);
      break;

    case kAsciiModeSwitchCommitText:
      ctx->ConfirmCurrentSelection();
      raw_keys_.clear();
      break;

    case kAsciiModeSwitchCommitCode:
      ctx->ClearNonConfirmedComposition();
      // 直接提交物理原始字串，不輸出經過轉譯的編碼
      engine_->CommitText(raw_keys_.empty() ? ctx->input() : raw_keys_);
      ctx->Clear();
      raw_keys_.clear();
      break;

    case kAsciiModeSwitchClear:
    case kAsciiModeSet:
    case kAsciiModeUnset:
      ctx->Clear();
      raw_keys_.clear();
      break;

    case kAsciiModeSwitchNoop:
    default:
      break;
  }

  ctx->set_option("ascii_mode", ascii_mode);
}

void AsciiComposer::TransitInlineAscii(Context* ctx, bool entering_ascii) {
  if (entering_ascii) {
    // 中文 -> 臨時西文：保存和弦快照，抹除候選菜單，換上實體字母
    cached_input_ = ctx->input();
    cached_raw_keys_ = raw_keys_;
    ctx->ClearNonConfirmedComposition();
    ctx->set_input(raw_keys_);
    is_inline_ascii_ = true;
    return;
  }

  // 臨時西文 -> 回中文：檢查西文字串是否被動過
  if (!cached_input_.empty() && raw_keys_ == cached_raw_keys_) {
    // 未修改：完美還原中文和弦
    ctx->set_input(cached_input_);
  } else {
    // 已修改：直接提交當前西文，乾淨復位
    engine_->CommitText(ctx->input());
    ctx->Clear();
    raw_keys_.clear();
  }
  cached_input_.clear();
  cached_raw_keys_.clear();
}

void AsciiComposer::OnContextUpdate(Context* ctx) {
  // 若不是臨時西文，一概無視
  if (!is_inline_ascii_) {
    return;
  }

  // 臨時西文結束時，收尾復位
  if (!ctx->IsComposing()) {
    is_inline_ascii_ = false;
    cached_input_.clear();
    cached_raw_keys_.clear();
    ctx->set_option("ascii_mode", false);
  }
}

void AsciiComposer::OnCommit(Context* ctx) {
  // 上屏結算時，物理賬本歸零
  raw_keys_.clear();
  cached_input_.clear();
  cached_raw_keys_.clear();
}

}  // namespace rime
