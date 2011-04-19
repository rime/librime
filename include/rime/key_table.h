// vim: set sw=2 sts=2 et:
// encoding: utf-8

#include <X11/keysym.h>

typedef enum {
  RIME_SHIFT_MASK    = 1 << 0,
  RIME_LOCK_MASK     = 1 << 1,
  RIME_CONTROL_MASK  = 1 << 2,
  RIME_MOD1_MASK     = 1 << 3,
  RIME_MOD2_MASK     = 1 << 4,
  RIME_MOD3_MASK     = 1 << 5,
  RIME_MOD4_MASK     = 1 << 6,
  RIME_MOD5_MASK     = 1 << 7,
  RIME_BUTTON1_MASK  = 1 << 8,
  RIME_BUTTON2_MASK  = 1 << 9,
  RIME_BUTTON3_MASK  = 1 << 10,
  RIME_BUTTON4_MASK  = 1 << 11,
  RIME_BUTTON5_MASK  = 1 << 12,

  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 23 are currently unused. Bit 29 is used internally.
   */

  /* ibus :) mask */
  RIME_HANDLED_MASK  = 1 << 24,
  RIME_FORWARD_MASK  = 1 << 25,
  RIME_IGNORED_MASK  = RIME_FORWARD_MASK,

  RIME_SUPER_MASK    = 1 << 26,
  RIME_HYPER_MASK    = 1 << 27,
  RIME_META_MASK     = 1 << 28,

  RIME_RELEASE_MASK  = 1 << 30,

  RIME_MODIFIER_MASK = 0x5f001fff
} RimeModifierType;

// 给定modifier文字，返回马赛克值
// 例如 GetModifierByName("Alt") == (1 << 3)
// 如果不认得所给的键名，返回 0
int GetModifierByName(const char *name);

// 给一个数值，取得最低的非0位所对应的modifier文字
// 例如 GetModifierName(12) == "Control"
// 取不到则返回 NULL
const char* GetModifierName(int modifier);

// 由键名取得键值
// 查无此键则返回 XK_VoidSymbol
int GetKeycodeByName(const char *name);

// 由键值取得键名
// 不认得此键，则返回 NULL
const char* GetKeyName(int keycode);

