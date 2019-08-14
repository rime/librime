//
// Copyright RIME Developers
// Distributed under the BSD License
//
// 2014-03-31 Chongyu Zhu <i@lembacon.com>
//
#include <stdint.h> // for uint32_t
#include <utf8.h>
#include <rime/candidate.h>
#include <rime/common.h>
#include <rime/context.h>
#include <rime/engine.h>
#include <rime/dict/vocabulary.h>
#include <rime/gear/charset_filter.h>
#include <boost/locale/encoding.hpp>
#include <boost/algorithm/string.hpp>


namespace rime {

bool is_extended_cjk(uint32_t ch)
{
  if ((ch >= 0x3400 && ch <= 0x4DBF) ||    // CJK Unified Ideographs Extension A
      (ch >= 0x20000 && ch <= 0x2A6DF) ||  // CJK Unified Ideographs Extension B
      (ch >= 0x2A700 && ch <= 0x2B73F) ||  // CJK Unified Ideographs Extension C
      (ch >= 0x2B740 && ch <= 0x2B81F) ||  // CJK Unified Ideographs Extension D
      (ch >= 0x2B820 && ch <= 0x2CEAF) ||  // CJK Unified Ideographs Extension E
      (ch >= 0x2CEB0 && ch <= 0x2EBEF) ||  // CJK Unified Ideographs Extension F
      (ch >= 0x2F800 && ch <= 0x2FA1F))    // CJK Compatibility Ideographs Supplement
    return true;

  return false;
}

bool contains_extended_cjk(const string& text)
{
  const char *p = text.c_str();
  uint32_t ch;

  while ((ch = utf8::unchecked::next(p)) != 0) {
    if (is_extended_cjk(ch)) {
      return true;
    }
  }

  return false;
}

static bool is_emoji(uint32_t ch)
{

  // emoji data from http://www.unicode.org/Public/emoji/12.0/emoji-data.txt
  if ((ch == 0x0023) ||
      (ch == 0x002A) ||
      (ch >= 0x0030 && ch <= 0x0039) ||
      (ch == 0x00A9) ||
      (ch == 0x00AE) ||
      (ch == 0x203C) ||
      (ch == 0x2049) ||
      (ch == 0x2122) ||
      (ch == 0x2139) ||
      (ch >= 0x2194 && ch <= 0x2199) ||
      (ch >= 0x21A9 && ch <= 0x21AA) ||
      (ch >= 0x231A && ch <= 0x231B) ||
      (ch == 0x2328) ||
      (ch == 0x23CF) ||
      (ch >= 0x23E9 && ch <= 0x23F3) ||
      (ch >= 0x23F8 && ch <= 0x23FA) ||
      (ch == 0x24C2) ||
      (ch >= 0x25AA && ch <= 0x25AB) ||
      (ch == 0x25B6) ||
      (ch == 0x25C0) ||
      (ch >= 0x25FB && ch <= 0x25FE) ||
      (ch >= 0x2600 && ch <= 0x2604) ||
      (ch == 0x260E) ||
      (ch == 0x2611) ||
      (ch >= 0x2614 && ch <= 0x2615) ||
      (ch == 0x2618) ||
      (ch == 0x261D) ||
      (ch == 0x2620) ||
      (ch >= 0x2622 && ch <= 0x2623) ||
      (ch == 0x2626) ||
      (ch == 0x262A) ||
      (ch >= 0x262E && ch <= 0x262F) ||
      (ch >= 0x2638 && ch <= 0x263A) ||
      (ch == 0x2640) ||
      (ch == 0x2642) ||
      (ch >= 0x2648 && ch <= 0x2653) ||
      (ch >= 0x265F && ch <= 0x2660) ||
      (ch == 0x2663) ||
      (ch >= 0x2665 && ch <= 0x2666) ||
      (ch == 0x2668) ||
      (ch == 0x267B) ||
      (ch >= 0x267E && ch <= 0x267F) ||
      (ch >= 0x2692 && ch <= 0x2697) ||
      (ch == 0x2699) ||
      (ch >= 0x269B && ch <= 0x269C) ||
      (ch >= 0x26A0 && ch <= 0x26A1) ||
      (ch >= 0x26AA && ch <= 0x26AB) ||
      (ch >= 0x26B0 && ch <= 0x26B1) ||
      (ch >= 0x26BD && ch <= 0x26BE) ||
      (ch >= 0x26C4 && ch <= 0x26C5) ||
      (ch == 0x26C8) ||
      (ch == 0x26CE) ||
      (ch == 0x26CF) ||
      (ch == 0x26D1) ||
      (ch >= 0x26D3 && ch <= 0x26D4) ||
      (ch >= 0x26E9 && ch <= 0x26EA) ||
      (ch >= 0x26F0 && ch <= 0x26F5) ||
      (ch >= 0x26F7 && ch <= 0x26FA) ||
      (ch == 0x26FD) ||
      (ch == 0x2702) ||
      (ch == 0x2705) ||
      (ch >= 0x2708 && ch <= 0x2709) ||
      (ch >= 0x270A && ch <= 0x270B) ||
      (ch >= 0x270C && ch <= 0x270D) ||
      (ch == 0x270F) ||
      (ch == 0x2712) ||
      (ch == 0x2714) ||
      (ch == 0x2716) ||
      (ch == 0x271D) ||
      (ch == 0x2721) ||
      (ch == 0x2728) ||
      (ch >= 0x2733 && ch <= 0x2734) ||
      (ch == 0x2744) ||
      (ch == 0x2747) ||
      (ch == 0x274C) ||
      (ch == 0x274E) ||
      (ch >= 0x2753 && ch <= 0x2755) ||
      (ch == 0x2757) ||
      (ch >= 0x2763 && ch <= 0x2764) ||
      (ch >= 0x2795 && ch <= 0x2797) ||
      (ch == 0x27A1) ||
      (ch == 0x27B0) ||
      (ch == 0x27BF) ||
      (ch >= 0x2934 && ch <= 0x2935) ||
      (ch >= 0x2B05 && ch <= 0x2B07) ||
      (ch >= 0x2B1B && ch <= 0x2B1C) ||
      (ch == 0x2B50) ||
      (ch == 0x2B55) ||
      (ch == 0x3030) ||
      (ch == 0x303D) ||
      (ch == 0x3297) ||
      (ch == 0x3299) ||
      (ch == 0x1F004) ||
      (ch == 0x1F0CF) ||
      (ch >= 0x1F170 && ch <= 0x1F171) ||
      (ch == 0x1F17E) ||
      (ch == 0x1F17F) ||
      (ch == 0x1F18E) ||
      (ch >= 0x1F191 && ch <= 0x1F19A) ||
      (ch >= 0x1F1E6 && ch <= 0x1F1FF) ||
      (ch >= 0x1F201 && ch <= 0x1F202) ||
      (ch == 0x1F21A) ||
      (ch == 0x1F22F) ||
      (ch >= 0x1F232 && ch <= 0x1F23A) ||
      (ch >= 0x1F250 && ch <= 0x1F251) ||
      (ch >= 0x1F300 && ch <= 0x1F320) ||
      (ch == 0x1F321) ||
      (ch >= 0x1F324 && ch <= 0x1F32C) ||
      (ch >= 0x1F32D && ch <= 0x1F32F) ||
      (ch >= 0x1F330 && ch <= 0x1F335) ||
      (ch == 0x1F336) ||
      (ch >= 0x1F337 && ch <= 0x1F37C) ||
      (ch == 0x1F37D) ||
      (ch >= 0x1F37E && ch <= 0x1F37F) ||
      (ch >= 0x1F380 && ch <= 0x1F393) ||
      (ch >= 0x1F396 && ch <= 0x1F397) ||
      (ch >= 0x1F399 && ch <= 0x1F39B) ||
      (ch >= 0x1F39E && ch <= 0x1F39F) ||
      (ch >= 0x1F3A0 && ch <= 0x1F3C4) ||
      (ch == 0x1F3C5) ||
      (ch >= 0x1F3C6 && ch <= 0x1F3CA) ||
      (ch >= 0x1F3CB && ch <= 0x1F3CE) ||
      (ch >= 0x1F3CF && ch <= 0x1F3D3) ||
      (ch >= 0x1F3D4 && ch <= 0x1F3DF) ||
      (ch >= 0x1F3E0 && ch <= 0x1F3F0) ||
      (ch >= 0x1F3F3 && ch <= 0x1F3F5) ||
      (ch == 0x1F3F7) ||
      (ch >= 0x1F3F8 && ch <= 0x1F3FF) ||
      (ch >= 0x1F400 && ch <= 0x1F43E) ||
      (ch == 0x1F43F) ||
      (ch == 0x1F440) ||
      (ch == 0x1F441) ||
      (ch >= 0x1F442 && ch <= 0x1F4F7) ||
      (ch == 0x1F4F8) ||
      (ch >= 0x1F4F9 && ch <= 0x1F4FC) ||
      (ch == 0x1F4FD) ||
      (ch == 0x1F4FF) ||
      (ch >= 0x1F500 && ch <= 0x1F53D) ||
      (ch >= 0x1F549 && ch <= 0x1F54A) ||
      (ch >= 0x1F54B && ch <= 0x1F54E) ||
      (ch >= 0x1F550 && ch <= 0x1F567) ||
      (ch >= 0x1F56F && ch <= 0x1F570) ||
      (ch >= 0x1F573 && ch <= 0x1F579) ||
      (ch == 0x1F57A) ||
      (ch == 0x1F587) ||
      (ch >= 0x1F58A && ch <= 0x1F58D) ||
      (ch == 0x1F590) ||
      (ch >= 0x1F595 && ch <= 0x1F596) ||
      (ch == 0x1F5A4) ||
      (ch == 0x1F5A5) ||
      (ch == 0x1F5A8) ||
      (ch >= 0x1F5B1 && ch <= 0x1F5B2) ||
      (ch == 0x1F5BC) ||
      (ch >= 0x1F5C2 && ch <= 0x1F5C4) ||
      (ch >= 0x1F5D1 && ch <= 0x1F5D3) ||
      (ch >= 0x1F5DC && ch <= 0x1F5DE) ||
      (ch == 0x1F5E1) ||
      (ch == 0x1F5E3) ||
      (ch == 0x1F5E8) ||
      (ch == 0x1F5EF) ||
      (ch == 0x1F5F3) ||
      (ch == 0x1F5FA) ||
      (ch >= 0x1F5FB && ch <= 0x1F5FF) ||
      (ch == 0x1F600) ||
      (ch >= 0x1F601 && ch <= 0x1F610) ||
      (ch == 0x1F611) ||
      (ch >= 0x1F612 && ch <= 0x1F614) ||
      (ch == 0x1F615) ||
      (ch == 0x1F616) ||
      (ch == 0x1F617) ||
      (ch == 0x1F618) ||
      (ch == 0x1F619) ||
      (ch == 0x1F61A) ||
      (ch == 0x1F61B) ||
      (ch >= 0x1F61C && ch <= 0x1F61E) ||
      (ch == 0x1F61F) ||
      (ch >= 0x1F620 && ch <= 0x1F625) ||
      (ch >= 0x1F626 && ch <= 0x1F627) ||
      (ch >= 0x1F628 && ch <= 0x1F62B) ||
      (ch == 0x1F62C) ||
      (ch == 0x1F62D) ||
      (ch >= 0x1F62E && ch <= 0x1F62F) ||
      (ch >= 0x1F630 && ch <= 0x1F633) ||
      (ch == 0x1F634) ||
      (ch >= 0x1F635 && ch <= 0x1F640) ||
      (ch >= 0x1F641 && ch <= 0x1F642) ||
      (ch >= 0x1F643 && ch <= 0x1F644) ||
      (ch >= 0x1F645 && ch <= 0x1F64F) ||
      (ch >= 0x1F680 && ch <= 0x1F6C5) ||
      (ch >= 0x1F6CB && ch <= 0x1F6CF) ||
      (ch == 0x1F6D0) ||
      (ch >= 0x1F6D1 && ch <= 0x1F6D2) ||
      (ch == 0x1F6D5) ||
      (ch >= 0x1F6E0 && ch <= 0x1F6E5) ||
      (ch == 0x1F6E9) ||
      (ch >= 0x1F6EB && ch <= 0x1F6EC) ||
      (ch == 0x1F6F0) ||
      (ch == 0x1F6F3) ||
      (ch >= 0x1F6F4 && ch <= 0x1F6F6) ||
      (ch >= 0x1F6F7 && ch <= 0x1F6F8) ||
      (ch == 0x1F6F9) ||
      (ch == 0x1F6FA) ||
      (ch >= 0x1F7E0 && ch <= 0x1F7EB) ||
      (ch >= 0x1F90D && ch <= 0x1F90F) ||
      (ch >= 0x1F910 && ch <= 0x1F918) ||
      (ch >= 0x1F919 && ch <= 0x1F91E) ||
      (ch == 0x1F91F) ||
      (ch >= 0x1F920 && ch <= 0x1F927) ||
      (ch >= 0x1F928 && ch <= 0x1F92F) ||
      (ch == 0x1F930) ||
      (ch >= 0x1F931 && ch <= 0x1F932) ||
      (ch >= 0x1F933 && ch <= 0x1F93A) ||
      (ch >= 0x1F93C && ch <= 0x1F93E) ||
      (ch == 0x1F93F) ||
      (ch >= 0x1F940 && ch <= 0x1F945) ||
      (ch >= 0x1F947 && ch <= 0x1F94B) ||
      (ch == 0x1F94C) ||
      (ch >= 0x1F94D && ch <= 0x1F94F) ||
      (ch >= 0x1F950 && ch <= 0x1F95E) ||
      (ch >= 0x1F95F && ch <= 0x1F96B) ||
      (ch >= 0x1F96C && ch <= 0x1F970) ||
      (ch == 0x1F971) ||
      (ch >= 0x1F973 && ch <= 0x1F976) ||
      (ch == 0x1F97A) ||
      (ch == 0x1F97B) ||
      (ch >= 0x1F97C && ch <= 0x1F97F) ||
      (ch >= 0x1F980 && ch <= 0x1F984) ||
      (ch >= 0x1F985 && ch <= 0x1F991) ||
      (ch >= 0x1F992 && ch <= 0x1F997) ||
      (ch >= 0x1F998 && ch <= 0x1F9A2) ||
      (ch >= 0x1F9A5 && ch <= 0x1F9AA) ||
      (ch >= 0x1F9AE && ch <= 0x1F9AF) ||
      (ch >= 0x1F9B0 && ch <= 0x1F9B9) ||
      (ch >= 0x1F9BA && ch <= 0x1F9BF) ||
      (ch == 0x1F9C0) ||
      (ch >= 0x1F9C1 && ch <= 0x1F9C2) ||
      (ch >= 0x1F9C3 && ch <= 0x1F9CA) ||
      (ch >= 0x1F9CD && ch <= 0x1F9CF) ||
      (ch >= 0x1F9D0 && ch <= 0x1F9E6) ||
      (ch >= 0x1F9E7 && ch <= 0x1F9FF) ||
      (ch >= 0x1FA70 && ch <= 0x1FA73) ||
      (ch >= 0x1FA78 && ch <= 0x1FA7A) ||
      (ch >= 0x1FA80 && ch <= 0x1FA82) ||
      (ch >= 0x1FA90 && ch <= 0x1FA95) ||
      (ch >= 0x231A && ch <= 0x231B) ||
      (ch >= 0x23E9 && ch <= 0x23EC) ||
      (ch == 0x23F0) ||
      (ch == 0x23F3) ||
      (ch >= 0x25FD && ch <= 0x25FE) ||
      (ch >= 0x2614 && ch <= 0x2615) ||
      (ch >= 0x2648 && ch <= 0x2653) ||
      (ch == 0x267F) ||
      (ch == 0x2693) ||
      (ch == 0x26A1) ||
      (ch >= 0x26AA && ch <= 0x26AB) ||
      (ch >= 0x26BD && ch <= 0x26BE) ||
      (ch >= 0x26C4 && ch <= 0x26C5) ||
      (ch == 0x26CE) ||
      (ch == 0x26D4) ||
      (ch == 0x26EA) ||
      (ch >= 0x26F2 && ch <= 0x26F3) ||
      (ch == 0x26F5) ||
      (ch == 0x26FA) ||
      (ch == 0x26FD) ||
      (ch == 0x2705) ||
      (ch >= 0x270A && ch <= 0x270B) ||
      (ch == 0x2728) ||
      (ch == 0x274C) ||
      (ch == 0x274E) ||
      (ch >= 0x2753 && ch <= 0x2755) ||
      (ch == 0x2757) ||
      (ch >= 0x2795 && ch <= 0x2797) ||
      (ch == 0x27B0) ||
      (ch == 0x27BF) ||
      (ch >= 0x2B1B && ch <= 0x2B1C) ||
      (ch == 0x2B50) ||
      (ch == 0x2B55) ||
      (ch == 0x1F004) ||
      (ch == 0x1F0CF) ||
      (ch == 0x1F18E) ||
      (ch >= 0x1F191 && ch <= 0x1F19A) ||
      (ch >= 0x1F1E6 && ch <= 0x1F1FF) ||
      (ch == 0x1F201) ||
      (ch == 0x1F21A) ||
      (ch == 0x1F22F) ||
      (ch >= 0x1F232 && ch <= 0x1F236) ||
      (ch >= 0x1F238 && ch <= 0x1F23A) ||
      (ch >= 0x1F250 && ch <= 0x1F251) ||
      (ch >= 0x1F300 && ch <= 0x1F320) ||
      (ch >= 0x1F32D && ch <= 0x1F32F) ||
      (ch >= 0x1F330 && ch <= 0x1F335) ||
      (ch >= 0x1F337 && ch <= 0x1F37C) ||
      (ch >= 0x1F37E && ch <= 0x1F37F) ||
      (ch >= 0x1F380 && ch <= 0x1F393) ||
      (ch >= 0x1F3A0 && ch <= 0x1F3C4) ||
      (ch == 0x1F3C5) ||
      (ch >= 0x1F3C6 && ch <= 0x1F3CA) ||
      (ch >= 0x1F3CF && ch <= 0x1F3D3) ||
      (ch >= 0x1F3E0 && ch <= 0x1F3F0) ||
      (ch == 0x1F3F4) ||
      (ch >= 0x1F3F8 && ch <= 0x1F3FF) ||
      (ch >= 0x1F400 && ch <= 0x1F43E) ||
      (ch == 0x1F440) ||
      (ch >= 0x1F442 && ch <= 0x1F4F7) ||
      (ch == 0x1F4F8) ||
      (ch >= 0x1F4F9 && ch <= 0x1F4FC) ||
      (ch == 0x1F4FF) ||
      (ch >= 0x1F500 && ch <= 0x1F53D) ||
      (ch >= 0x1F54B && ch <= 0x1F54E) ||
      (ch >= 0x1F550 && ch <= 0x1F567) ||
      (ch == 0x1F57A) ||
      (ch >= 0x1F595 && ch <= 0x1F596) ||
      (ch == 0x1F5A4) ||
      (ch >= 0x1F5FB && ch <= 0x1F5FF) ||
      (ch == 0x1F600) ||
      (ch >= 0x1F601 && ch <= 0x1F610) ||
      (ch == 0x1F611) ||
      (ch >= 0x1F612 && ch <= 0x1F614) ||
      (ch == 0x1F615) ||
      (ch == 0x1F616) ||
      (ch == 0x1F617) ||
      (ch == 0x1F618) ||
      (ch == 0x1F619) ||
      (ch == 0x1F61A) ||
      (ch == 0x1F61B) ||
      (ch >= 0x1F61C && ch <= 0x1F61E) ||
      (ch == 0x1F61F) ||
      (ch >= 0x1F620 && ch <= 0x1F625) ||
      (ch >= 0x1F626 && ch <= 0x1F627) ||
      (ch >= 0x1F628 && ch <= 0x1F62B) ||
      (ch == 0x1F62C) ||
      (ch == 0x1F62D) ||
      (ch >= 0x1F62E && ch <= 0x1F62F) ||
      (ch >= 0x1F630 && ch <= 0x1F633) ||
      (ch == 0x1F634) ||
      (ch >= 0x1F635 && ch <= 0x1F640) ||
      (ch >= 0x1F641 && ch <= 0x1F642) ||
      (ch >= 0x1F643 && ch <= 0x1F644) ||
      (ch >= 0x1F645 && ch <= 0x1F64F) ||
      (ch >= 0x1F680 && ch <= 0x1F6C5) ||
      (ch == 0x1F6CC) ||
      (ch == 0x1F6D0) ||
      (ch >= 0x1F6D1 && ch <= 0x1F6D2) ||
      (ch == 0x1F6D5) ||
      (ch >= 0x1F6EB && ch <= 0x1F6EC) ||
      (ch >= 0x1F6F4 && ch <= 0x1F6F6) ||
      (ch >= 0x1F6F7 && ch <= 0x1F6F8) ||
      (ch == 0x1F6F9) ||
      (ch == 0x1F6FA) ||
      (ch >= 0x1F7E0 && ch <= 0x1F7EB) ||
      (ch >= 0x1F90D && ch <= 0x1F90F) ||
      (ch >= 0x1F910 && ch <= 0x1F918) ||
      (ch >= 0x1F919 && ch <= 0x1F91E) ||
      (ch == 0x1F91F) ||
      (ch >= 0x1F920 && ch <= 0x1F927) ||
      (ch >= 0x1F928 && ch <= 0x1F92F) ||
      (ch == 0x1F930) ||
      (ch >= 0x1F931 && ch <= 0x1F932) ||
      (ch >= 0x1F933 && ch <= 0x1F93A) ||
      (ch >= 0x1F93C && ch <= 0x1F93E) ||
      (ch == 0x1F93F) ||
      (ch >= 0x1F940 && ch <= 0x1F945) ||
      (ch >= 0x1F947 && ch <= 0x1F94B) ||
      (ch == 0x1F94C) ||
      (ch >= 0x1F94D && ch <= 0x1F94F) ||
      (ch >= 0x1F950 && ch <= 0x1F95E) ||
      (ch >= 0x1F95F && ch <= 0x1F96B) ||
      (ch >= 0x1F96C && ch <= 0x1F970) ||
      (ch == 0x1F971) ||
      (ch >= 0x1F973 && ch <= 0x1F976) ||
      (ch == 0x1F97A) ||
      (ch == 0x1F97B) ||
      (ch >= 0x1F97C && ch <= 0x1F97F) ||
      (ch >= 0x1F980 && ch <= 0x1F984) ||
      (ch >= 0x1F985 && ch <= 0x1F991) ||
      (ch >= 0x1F992 && ch <= 0x1F997) ||
      (ch >= 0x1F998 && ch <= 0x1F9A2) ||
      (ch >= 0x1F9A5 && ch <= 0x1F9AA) ||
      (ch >= 0x1F9AE && ch <= 0x1F9AF) ||
      (ch >= 0x1F9B0 && ch <= 0x1F9B9) ||
      (ch >= 0x1F9BA && ch <= 0x1F9BF) ||
      (ch == 0x1F9C0) ||
      (ch >= 0x1F9C1 && ch <= 0x1F9C2) ||
      (ch >= 0x1F9C3 && ch <= 0x1F9CA) ||
      (ch >= 0x1F9CD && ch <= 0x1F9CF) ||
      (ch >= 0x1F9D0 && ch <= 0x1F9E6) ||
      (ch >= 0x1F9E7 && ch <= 0x1F9FF) ||
      (ch >= 0x1FA70 && ch <= 0x1FA73) ||
      (ch >= 0x1FA78 && ch <= 0x1FA7A) ||
      (ch >= 0x1FA80 && ch <= 0x1FA82) ||
      (ch >= 0x1FA90 && ch <= 0x1FA95) ||
      (ch >= 0x1F3FB && ch <= 0x1F3FF) ||
      (ch == 0x261D) ||
      (ch == 0x26F9) ||
      (ch >= 0x270A && ch <= 0x270B) ||
      (ch >= 0x270C && ch <= 0x270D) ||
      (ch == 0x1F385) ||
      (ch >= 0x1F3C2 && ch <= 0x1F3C4) ||
      (ch == 0x1F3C7) ||
      (ch == 0x1F3CA) ||
      (ch >= 0x1F3CB && ch <= 0x1F3CC) ||
      (ch >= 0x1F442 && ch <= 0x1F443) ||
      (ch >= 0x1F446 && ch <= 0x1F450) ||
      (ch >= 0x1F466 && ch <= 0x1F478) ||
      (ch == 0x1F47C) ||
      (ch >= 0x1F481 && ch <= 0x1F483) ||
      (ch >= 0x1F485 && ch <= 0x1F487) ||
      (ch == 0x1F48F) ||
      (ch == 0x1F491) ||
      (ch == 0x1F4AA) ||
      (ch >= 0x1F574 && ch <= 0x1F575) ||
      (ch == 0x1F57A) ||
      (ch == 0x1F590) ||
      (ch >= 0x1F595 && ch <= 0x1F596) ||
      (ch >= 0x1F645 && ch <= 0x1F647) ||
      (ch >= 0x1F64B && ch <= 0x1F64F) ||
      (ch == 0x1F6A3) ||
      (ch >= 0x1F6B4 && ch <= 0x1F6B6) ||
      (ch == 0x1F6C0) ||
      (ch == 0x1F6CC) ||
      (ch == 0x1F90F) ||
      (ch == 0x1F918) ||
      (ch >= 0x1F919 && ch <= 0x1F91E) ||
      (ch == 0x1F91F) ||
      (ch == 0x1F926) ||
      (ch == 0x1F930) ||
      (ch >= 0x1F931 && ch <= 0x1F932) ||
      (ch >= 0x1F933 && ch <= 0x1F939) ||
      (ch >= 0x1F93C && ch <= 0x1F93E) ||
      (ch >= 0x1F9B5 && ch <= 0x1F9B6) ||
      (ch >= 0x1F9B8 && ch <= 0x1F9B9) ||
      (ch == 0x1F9BB) ||
      (ch >= 0x1F9CD && ch <= 0x1F9CF) ||
      (ch >= 0x1F9D1 && ch <= 0x1F9DD) ||
      (ch == 0x0023) ||
      (ch == 0x002A) ||
      (ch >= 0x0030 && ch <= 0x0039) ||
      (ch == 0x200D) ||
      (ch == 0x20E3) ||
      (ch == 0xFE0F) ||
      (ch >= 0x1F1E6 && ch <= 0x1F1FF) ||
      (ch >= 0x1F3FB && ch <= 0x1F3FF) ||
      (ch >= 0x1F9B0 && ch <= 0x1F9B3) ||
      (ch >= 0xE0020 && ch <= 0xE007F) ||
      (ch == 0x00A9) ||
      (ch == 0x00AE) ||
      (ch == 0x203C) ||
      (ch == 0x2049) ||
      (ch == 0x2122) ||
      (ch == 0x2139) ||
      (ch >= 0x2194 && ch <= 0x2199) ||
      (ch >= 0x21A9 && ch <= 0x21AA) ||
      (ch >= 0x231A && ch <= 0x231B) ||
      (ch == 0x2328) ||
      (ch == 0x2388) ||
      (ch == 0x23CF) ||
      (ch >= 0x23E9 && ch <= 0x23F3) ||
      (ch >= 0x23F8 && ch <= 0x23FA) ||
      (ch == 0x24C2) ||
      (ch >= 0x25AA && ch <= 0x25AB) ||
      (ch == 0x25B6) ||
      (ch == 0x25C0) ||
      (ch >= 0x25FB && ch <= 0x25FE) ||
      (ch >= 0x2600 && ch <= 0x2605) ||
      (ch >= 0x2607 && ch <= 0x2612) ||
      (ch >= 0x2614 && ch <= 0x2615) ||
      (ch >= 0x2616 && ch <= 0x2617) ||
      (ch == 0x2618) ||
      (ch == 0x2619) ||
      (ch >= 0x261A && ch <= 0x266F) ||
      (ch >= 0x2670 && ch <= 0x2671) ||
      (ch >= 0x2672 && ch <= 0x267D) ||
      (ch >= 0x267E && ch <= 0x267F) ||
      (ch >= 0x2680 && ch <= 0x2685) ||
      (ch >= 0x2690 && ch <= 0x2691) ||
      (ch >= 0x2692 && ch <= 0x269C) ||
      (ch == 0x269D) ||
      (ch >= 0x269E && ch <= 0x269F) ||
      (ch >= 0x26A0 && ch <= 0x26A1) ||
      (ch >= 0x26A2 && ch <= 0x26B1) ||
      (ch == 0x26B2) ||
      (ch >= 0x26B3 && ch <= 0x26BC) ||
      (ch >= 0x26BD && ch <= 0x26BF) ||
      (ch >= 0x26C0 && ch <= 0x26C3) ||
      (ch >= 0x26C4 && ch <= 0x26CD) ||
      (ch == 0x26CE) ||
      (ch >= 0x26CF && ch <= 0x26E1) ||
      (ch == 0x26E2) ||
      (ch == 0x26E3) ||
      (ch >= 0x26E4 && ch <= 0x26E7) ||
      (ch >= 0x26E8 && ch <= 0x26FF) ||
      (ch == 0x2700) ||
      (ch >= 0x2701 && ch <= 0x2704) ||
      (ch == 0x2705) ||
      (ch >= 0x2708 && ch <= 0x2709) ||
      (ch >= 0x270A && ch <= 0x270B) ||
      (ch >= 0x270C && ch <= 0x2712) ||
      (ch == 0x2714) ||
      (ch == 0x2716) ||
      (ch == 0x271D) ||
      (ch == 0x2721) ||
      (ch == 0x2728) ||
      (ch >= 0x2733 && ch <= 0x2734) ||
      (ch == 0x2744) ||
      (ch == 0x2747) ||
      (ch == 0x274C) ||
      (ch == 0x274E) ||
      (ch >= 0x2753 && ch <= 0x2755) ||
      (ch == 0x2757) ||
      (ch >= 0x2763 && ch <= 0x2767) ||
      (ch >= 0x2795 && ch <= 0x2797) ||
      (ch == 0x27A1) ||
      (ch == 0x27B0) ||
      (ch == 0x27BF) ||
      (ch >= 0x2934 && ch <= 0x2935) ||
      (ch >= 0x2B05 && ch <= 0x2B07) ||
      (ch >= 0x2B1B && ch <= 0x2B1C) ||
      (ch == 0x2B50) ||
      (ch == 0x2B55) ||
      (ch == 0x3030) ||
      (ch == 0x303D) ||
      (ch == 0x3297) ||
      (ch == 0x3299) ||
      (ch >= 0x1F000 && ch <= 0x1F02B) ||
      (ch >= 0x1F02C && ch <= 0x1F02F) ||
      (ch >= 0x1F030 && ch <= 0x1F093) ||
      (ch >= 0x1F094 && ch <= 0x1F09F) ||
      (ch >= 0x1F0A0 && ch <= 0x1F0AE) ||
      (ch >= 0x1F0AF && ch <= 0x1F0B0) ||
      (ch >= 0x1F0B1 && ch <= 0x1F0BE) ||
      (ch == 0x1F0BF) ||
      (ch == 0x1F0C0) ||
      (ch >= 0x1F0C1 && ch <= 0x1F0CF) ||
      (ch == 0x1F0D0) ||
      (ch >= 0x1F0D1 && ch <= 0x1F0DF) ||
      (ch >= 0x1F0E0 && ch <= 0x1F0F5) ||
      (ch >= 0x1F0F6 && ch <= 0x1F0FF) ||
      (ch >= 0x1F10D && ch <= 0x1F10F) ||
      (ch == 0x1F12F) ||
      (ch == 0x1F16C) ||
      (ch >= 0x1F16D && ch <= 0x1F16F) ||
      (ch >= 0x1F170 && ch <= 0x1F171) ||
      (ch == 0x1F17E) ||
      (ch == 0x1F17F) ||
      (ch == 0x1F18E) ||
      (ch >= 0x1F191 && ch <= 0x1F19A) ||
      (ch >= 0x1F1AD && ch <= 0x1F1E5) ||
      (ch >= 0x1F201 && ch <= 0x1F202) ||
      (ch >= 0x1F203 && ch <= 0x1F20F) ||
      (ch == 0x1F21A) ||
      (ch == 0x1F22F) ||
      (ch >= 0x1F232 && ch <= 0x1F23A) ||
      (ch >= 0x1F23C && ch <= 0x1F23F) ||
      (ch >= 0x1F249 && ch <= 0x1F24F) ||
      (ch >= 0x1F250 && ch <= 0x1F251) ||
      (ch >= 0x1F252 && ch <= 0x1F25F) ||
      (ch >= 0x1F260 && ch <= 0x1F265) ||
      (ch >= 0x1F266 && ch <= 0x1F2FF) ||
      (ch >= 0x1F300 && ch <= 0x1F320) ||
      (ch >= 0x1F321 && ch <= 0x1F32C) ||
      (ch >= 0x1F32D && ch <= 0x1F32F) ||
      (ch >= 0x1F330 && ch <= 0x1F335) ||
      (ch == 0x1F336) ||
      (ch >= 0x1F337 && ch <= 0x1F37C) ||
      (ch == 0x1F37D) ||
      (ch >= 0x1F37E && ch <= 0x1F37F) ||
      (ch >= 0x1F380 && ch <= 0x1F393) ||
      (ch >= 0x1F394 && ch <= 0x1F39F) ||
      (ch >= 0x1F3A0 && ch <= 0x1F3C4) ||
      (ch == 0x1F3C5) ||
      (ch >= 0x1F3C6 && ch <= 0x1F3CA) ||
      (ch >= 0x1F3CB && ch <= 0x1F3CE) ||
      (ch >= 0x1F3CF && ch <= 0x1F3D3) ||
      (ch >= 0x1F3D4 && ch <= 0x1F3DF) ||
      (ch >= 0x1F3E0 && ch <= 0x1F3F0) ||
      (ch >= 0x1F3F1 && ch <= 0x1F3F7) ||
      (ch >= 0x1F3F8 && ch <= 0x1F3FA) ||
      (ch >= 0x1F400 && ch <= 0x1F43E) ||
      (ch == 0x1F43F) ||
      (ch == 0x1F440) ||
      (ch == 0x1F441) ||
      (ch >= 0x1F442 && ch <= 0x1F4F7) ||
      (ch == 0x1F4F8) ||
      (ch >= 0x1F4F9 && ch <= 0x1F4FC) ||
      (ch >= 0x1F4FD && ch <= 0x1F4FE) ||
      (ch == 0x1F4FF) ||
      (ch >= 0x1F500 && ch <= 0x1F53D) ||
      (ch >= 0x1F546 && ch <= 0x1F54A) ||
      (ch >= 0x1F54B && ch <= 0x1F54F) ||
      (ch >= 0x1F550 && ch <= 0x1F567) ||
      (ch >= 0x1F568 && ch <= 0x1F579) ||
      (ch == 0x1F57A) ||
      (ch >= 0x1F57B && ch <= 0x1F5A3) ||
      (ch == 0x1F5A4) ||
      (ch >= 0x1F5A5 && ch <= 0x1F5FA) ||
      (ch >= 0x1F5FB && ch <= 0x1F5FF) ||
      (ch == 0x1F600) ||
      (ch >= 0x1F601 && ch <= 0x1F610) ||
      (ch == 0x1F611) ||
      (ch >= 0x1F612 && ch <= 0x1F614) ||
      (ch == 0x1F615) ||
      (ch == 0x1F616) ||
      (ch == 0x1F617) ||
      (ch == 0x1F618) ||
      (ch == 0x1F619) ||
      (ch == 0x1F61A) ||
      (ch == 0x1F61B) ||
      (ch >= 0x1F61C && ch <= 0x1F61E) ||
      (ch == 0x1F61F) ||
      (ch >= 0x1F620 && ch <= 0x1F625) ||
      (ch >= 0x1F626 && ch <= 0x1F627) ||
      (ch >= 0x1F628 && ch <= 0x1F62B) ||
      (ch == 0x1F62C) ||
      (ch == 0x1F62D) ||
      (ch >= 0x1F62E && ch <= 0x1F62F) ||
      (ch >= 0x1F630 && ch <= 0x1F633) ||
      (ch == 0x1F634) ||
      (ch >= 0x1F635 && ch <= 0x1F640) ||
      (ch >= 0x1F641 && ch <= 0x1F642) ||
      (ch >= 0x1F643 && ch <= 0x1F644) ||
      (ch >= 0x1F645 && ch <= 0x1F64F) ||
      (ch >= 0x1F680 && ch <= 0x1F6C5) ||
      (ch >= 0x1F6C6 && ch <= 0x1F6CF) ||
      (ch == 0x1F6D0) ||
      (ch >= 0x1F6D1 && ch <= 0x1F6D2) ||
      (ch >= 0x1F6D3 && ch <= 0x1F6D4) ||
      (ch == 0x1F6D5) ||
      (ch >= 0x1F6D6 && ch <= 0x1F6DF) ||
      (ch >= 0x1F6E0 && ch <= 0x1F6EC) ||
      (ch >= 0x1F6ED && ch <= 0x1F6EF) ||
      (ch >= 0x1F6F0 && ch <= 0x1F6F3) ||
      (ch >= 0x1F6F4 && ch <= 0x1F6F6) ||
      (ch >= 0x1F6F7 && ch <= 0x1F6F8) ||
      (ch == 0x1F6F9) ||
      (ch == 0x1F6FA) ||
      (ch >= 0x1F6FB && ch <= 0x1F6FF) ||
      (ch >= 0x1F774 && ch <= 0x1F77F) ||
      (ch >= 0x1F7D5 && ch <= 0x1F7D8) ||
      (ch >= 0x1F7D9 && ch <= 0x1F7DF) ||
      (ch >= 0x1F7E0 && ch <= 0x1F7EB) ||
      (ch >= 0x1F7EC && ch <= 0x1F7FF) ||
      (ch >= 0x1F80C && ch <= 0x1F80F) ||
      (ch >= 0x1F848 && ch <= 0x1F84F) ||
      (ch >= 0x1F85A && ch <= 0x1F85F) ||
      (ch >= 0x1F888 && ch <= 0x1F88F) ||
      (ch >= 0x1F8AE && ch <= 0x1F8FF) ||
      (ch == 0x1F90C) ||
      (ch >= 0x1F90D && ch <= 0x1F90F) ||
      (ch >= 0x1F910 && ch <= 0x1F918) ||
      (ch >= 0x1F919 && ch <= 0x1F91E) ||
      (ch == 0x1F91F) ||
      (ch >= 0x1F920 && ch <= 0x1F927) ||
      (ch >= 0x1F928 && ch <= 0x1F92F) ||
      (ch == 0x1F930) ||
      (ch >= 0x1F931 && ch <= 0x1F932) ||
      (ch >= 0x1F933 && ch <= 0x1F93A) ||
      (ch >= 0x1F93C && ch <= 0x1F93E) ||
      (ch == 0x1F93F) ||
      (ch >= 0x1F940 && ch <= 0x1F945) ||
      (ch >= 0x1F947 && ch <= 0x1F94B) ||
      (ch == 0x1F94C) ||
      (ch >= 0x1F94D && ch <= 0x1F94F) ||
      (ch >= 0x1F950 && ch <= 0x1F95E) ||
      (ch >= 0x1F95F && ch <= 0x1F96B) ||
      (ch >= 0x1F96C && ch <= 0x1F970) ||
      (ch == 0x1F971) ||
      (ch == 0x1F972) ||
      (ch >= 0x1F973 && ch <= 0x1F976) ||
      (ch >= 0x1F977 && ch <= 0x1F979) ||
      (ch == 0x1F97A) ||
      (ch == 0x1F97B) ||
      (ch >= 0x1F97C && ch <= 0x1F97F) ||
      (ch >= 0x1F980 && ch <= 0x1F984) ||
      (ch >= 0x1F985 && ch <= 0x1F991) ||
      (ch >= 0x1F992 && ch <= 0x1F997) ||
      (ch >= 0x1F998 && ch <= 0x1F9A2) ||
      (ch >= 0x1F9A3 && ch <= 0x1F9A4) ||
      (ch >= 0x1F9A5 && ch <= 0x1F9AA) ||
      (ch >= 0x1F9AB && ch <= 0x1F9AD) ||
      (ch >= 0x1F9AE && ch <= 0x1F9AF) ||
      (ch >= 0x1F9B0 && ch <= 0x1F9B9) ||
      (ch >= 0x1F9BA && ch <= 0x1F9BF) ||
      (ch == 0x1F9C0) ||
      (ch >= 0x1F9C1 && ch <= 0x1F9C2) ||
      (ch >= 0x1F9C3 && ch <= 0x1F9CA) ||
      (ch >= 0x1F9CB && ch <= 0x1F9CC) ||
      (ch >= 0x1F9CD && ch <= 0x1F9CF) ||
      (ch >= 0x1F9D0 && ch <= 0x1F9E6) ||
      (ch >= 0x1F9E7 && ch <= 0x1F9FF) ||
      (ch >= 0x1FA00 && ch <= 0x1FA53) ||
      (ch >= 0x1FA54 && ch <= 0x1FA5F) ||
      (ch >= 0x1FA60 && ch <= 0x1FA6D) ||
      (ch >= 0x1FA6E && ch <= 0x1FA6F) ||
      (ch >= 0x1FA70 && ch <= 0x1FA73) ||
      (ch >= 0x1FA74 && ch <= 0x1FA77) ||
      (ch >= 0x1FA78 && ch <= 0x1FA7A) ||
      (ch >= 0x1FA7B && ch <= 0x1FA7F) ||
      (ch >= 0x1FA80 && ch <= 0x1FA82) ||
      (ch >= 0x1FA83 && ch <= 0x1FA8F) ||
      (ch >= 0x1FA90 && ch <= 0x1FA95) ||
      (ch >= 0x1FA96 && ch <= 0x1FFFD))
      return true;

  return false;
}

static bool is_all_emoji(const string& text)
{
  const char *p = text.c_str();
  uint32_t ch;

  while ((ch = utf8::unchecked::next(p)) != 0) {
    if (!is_emoji(ch)) {
      return false;
    }
  }

  return true;
}

// CharsetFilterTranslation

CharsetFilterTranslation::CharsetFilterTranslation(
    an<Translation> translation, const string& charset_with_parameter_)
    : translation_(translation), charset_with_parameter_(charset_with_parameter_) {
  LocateNextCandidate();
}

bool CharsetFilterTranslation::Next() {
  if (exhausted())
    return false;
  if (!translation_->Next()) {
    set_exhausted(true);
    return false;
  }
  return LocateNextCandidate();
}

an<Candidate> CharsetFilterTranslation::Peek() {
  return translation_->Peek();
}

bool CharsetFilterTranslation::LocateNextCandidate() {
  while (!translation_->exhausted()) {
    auto cand = translation_->Peek();
    if (cand && CharsetFilter::FilterText(cand->text(), charset_with_parameter_))
      return true;
    translation_->Next();
  }
  set_exhausted(true);
  return false;
}

// CharsetFilter

bool CharsetFilter::FilterText(const string& text, const string& charset_with_parameter) {
  if (charset_with_parameter.empty()) return !contains_extended_cjk(text);
  vector<string> charset_arguments_vector;
  boost::split(charset_arguments_vector, charset_with_parameter, boost::is_any_of("+"));
  bool is_emoji_enabled = false;
  if (std::find(charset_arguments_vector.begin(), charset_arguments_vector.end(), "emoji") != charset_arguments_vector.end()) {
	is_emoji_enabled = true;
  }
  if (is_emoji_enabled && is_all_emoji(text)) {
	return true;
  }

  try {
    const auto& charset = charset_arguments_vector[0];
    boost::locale::conv::from_utf(text, charset, boost::locale::conv::method_type::stop);
  }
  catch(boost::locale::conv::conversion_error const& /*ex*/) {
    return false;
  }
  catch(...) {
  }
  return true;
}

bool CharsetFilter::FilterDictEntry(an<DictEntry> entry) {
  return entry && FilterText(entry->text);
}

CharsetFilter::CharsetFilter(const Ticket& ticket)
    : Filter(ticket), TagMatching(ticket) {
}

an<Translation> CharsetFilter::Apply(
    an<Translation> translation, CandidateList* candidates) {
  if (name_space_.empty() && !engine_->context()->get_option("extended_charset")) {
    return New<CharsetFilterTranslation>(translation);
  }
  if (!name_space_.empty() && engine_->context()->get_option(name_space_)) {
    return New<CharsetFilterTranslation>(translation, name_space_);
  }
  return translation;
}

}  // namespace rime
