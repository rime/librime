// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-17 GONG Chen <chen.sst@gmail.com>
//
#ifndef RIME_KEY_TABLE_H_
#define RIME_KEY_TABLE_H_

#include <X11/keysym.h>

typedef enum {
  kShiftMask    = 1 << 0,
  kLockMask     = 1 << 1,
  kControlMask  = 1 << 2,
  kMod1Mask     = 1 << 3,
  kAltMask      = kMod1Mask,
  kMod2Mask     = 1 << 4,
  kMod3Mask     = 1 << 5,
  kMod4Mask     = 1 << 6,
  kMod5Mask     = 1 << 7,
  kButton1Mask  = 1 << 8,
  kButton2Mask  = 1 << 9,
  kButton3Mask  = 1 << 10,
  kButton4Mask  = 1 << 11,
  kButton5Mask  = 1 << 12,

  /* The next few modifiers are used by XKB, so we skip to the end.
   * Bits 15 - 23 are currently unused. Bit 29 is used internally.
   */

  /* ibus :) mask */
  kHandledMask  = 1 << 24,
  kForwardMask  = 1 << 25,
  kIgnoredMask  = kForwardMask,

  kSuperMask    = 1 << 26,
  kHyperMask    = 1 << 27,
  kMetaMask     = 1 << 28,

  kReleaseMask  = 1 << 30,

  kModifierMask = 0x5f001fff
} RimeModifier;

// 给定modifier文字，返回马赛克值
// 例如 RimeGetModifierByName("Alt") == (1 << 3)
// 如果不认得所给的键名，返回 0
int RimeGetModifierByName(const char *name);

// 给一个数值，取得最低的非0位所对应的modifier文字
// 例如 RimeGetModifierName(12) == "Control"
// 取不到则返回 NULL
const char* RimeGetModifierName(int modifier);

// 由键名取得键值
// 查无此键则返回 XK_VoidSymbol
int RimeGetKeycodeByName(const char *name);

// 由键值取得键名
// 不认得此键，则返回 NULL
const char* RimeGetKeyName(int keycode);

#endif  // RIME_KEY_TABLE_H_
