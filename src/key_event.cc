// vim: set sts=2 sw=2 et:
// encoding: utf-8
//
// Copyleft 2011 RIME Developers
// License: GPLv3
// 
// 2011-04-20 GONG Chen <chen.sst@gmail.com>
//
#include <string>
#include <sstream>
#include <rime/key_event.h>

namespace rime {

KeyEvent::KeyEvent(const std::string &repr) {
  if (!Parse(repr))
    keycode_ = modifier_ = 0;
}

const std::string KeyEvent::repr() const {
  // stringify modifiers
  std::ostringstream modifiers;
  if (modifier_) {
    int k = modifier_ & RIME_MODIFIER_MASK;
    const char *modifier_name = NULL;
    for (int i = 0; k; ++i, k >>= 1) {
      if (!(k & 1))
        continue;
      modifier_name = GetModifierName(k << i);
      if (modifier_name) {
        modifiers << modifier_name << '+';
      }
    }
  }
  // first lookup predefined key name
  const char *name = GetKeyName(keycode_);
  if (name)
    return modifiers.str() + name;
  // no name :-| return its hex value
  char value[9] = {0};
  if (keycode_ <= 0xffff) {
    sprintf(value, "0x%4x", keycode_);
  }
  else if (keycode_ <= 0xffffff) {
    sprintf(value, "0x%6x", keycode_);
  }
  else {
    return "(unknown)";  // invalid keycode
  }
  return modifiers.str() + value;
}

bool KeyEvent::Parse(const std::string &repr) {
  keycode_ = modifier_ = 0;
  if (repr.empty()) {
    return false;
  }
  if (repr.size() == 1) {
    keycode_ = static_cast<int>(repr[0]);
  }
  else {
    size_t start = 0;
    size_t found = 0;
    std::string token;
    int mask = 0;
    while ((found = repr.find('+', start)) != std::string::npos) {
      token = repr.substr(start, found - start);
      mask = GetModifierByName(token.c_str());
      if (mask) {
        modifier_ |= mask;
      }
      else {
        EZLOGGERPRINT("parse error: unrecognized modifier '%s'", token.c_str());
        return false;
      }
      start = found + 1;
    }
    token = repr.substr(start);
    keycode_ = GetKeycodeByName(token.c_str());
    if (keycode_ == XK_VoidSymbol) {
      EZLOGGERPRINT("parse error: unrecognized key '%s'", token.c_str());
      return false;
    }
  }
  return true;
}

KeySequence::KeySequence(const std::string &repr) {
  if (!Parse(repr))
    clear();
}

const std::string KeySequence::repr() const {
  std::ostringstream result;
  std::string k;
  for (const_iterator it = begin(); it != end(); ++it) {
    k = it->repr();
    if (k.size() == 1) {
      result << k;
    }
    else {
      result << '{' << k << '}';
    }
  }
  return result.str();
}

bool KeySequence::Parse(const std::string &repr) {
  clear();
  size_t n = repr.size();
  size_t start = 0;
  size_t len = 0;
  KeyEvent ke;
  for (size_t i = 0; i < n; ++i) {
    if (repr[i] == '{' && i + 1 < n) {
      start = i + 1;
      size_t j = repr.find('}', start);
      if (j == std::string::npos) {
        EZLOGGERPRINT("parse error: unparalleled brace in '%s'", repr.c_str());
        return false;
      }
      len = j - start;
      i = j;
    }
    else {
      start = i;
      len = 1;
    }
    if (!ke.Parse(repr.substr(start, len))) {
      EZLOGGERPRINT("parse error: unrecognized key sequence");
      return false;
    }
    push_back(ke);
  }
  return true;
}

}  // namespace rime
