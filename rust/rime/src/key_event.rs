// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/key_event.h and src/rime/key_event.cc to Rust.

use std::ops::{Deref, DerefMut};

use crate::key_table::{
    get_key_name, get_keycode_by_name, get_modifier_by_name, get_modifier_name,
    MODIFIER_MASK, XK_VOID_SYMBOL,
};

/// 按鍵，表示为 (keycode, modifier)。modifier 是一组 modifier mask 的按位或。
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct KeyEvent {
    keycode: i32,
    modifier: i32,
}

impl KeyEvent {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn with_keycode(keycode: i32, modifier: i32) -> Self {
        Self { keycode, modifier }
    }

    pub fn from_repr(repr: &str) -> Self {
        let mut ev = Self::new();
        if !ev.parse(repr) {
            ev.keycode = 0;
            ev.modifier = 0;
        }
        ev
    }

    pub fn keycode(&self) -> i32 {
        self.keycode
    }

    pub fn set_keycode(&mut self, value: i32) {
        self.keycode = value;
    }

    pub fn modifier(&self) -> i32 {
        self.modifier
    }

    pub fn set_modifier(&mut self, value: i32) {
        self.modifier = value;
    }

    pub fn shift(&self) -> bool {
        self.modifier & crate::key_table::SHIFT_MASK != 0
    }
    pub fn ctrl(&self) -> bool {
        self.modifier & crate::key_table::CONTROL_MASK != 0
    }
    pub fn alt(&self) -> bool {
        self.modifier & crate::key_table::ALT_MASK != 0
    }
    pub fn caps(&self) -> bool {
        self.modifier & crate::key_table::LOCK_MASK != 0
    }
    pub fn super_key(&self) -> bool {
        self.modifier & crate::key_table::SUPER_MASK != 0
    }
    pub fn release(&self) -> bool {
        self.modifier & crate::key_table::RELEASE_MASK != 0
    }

    /// 按鍵表示為形如「狀態+鍵名」的文字；若無鍵名，則以十六进制数形式的文字標識。
    pub fn repr(&self) -> String {
        // stringify modifiers (lowest bit first)
        let mut modifiers = String::new();
        let k = self.modifier & MODIFIER_MASK;
        for i in 0..31 {
            let bit = 1i32 << i;
            if k & bit != 0 {
                if let Some(name) = get_modifier_name(bit) {
                    modifiers.push_str(name);
                    modifiers.push('+');
                }
            }
        }
        if let Some(name) = get_key_name(self.keycode) {
            return modifiers + name;
        }
        if self.keycode <= 0xffff {
            format!("{}0x{:04x}", modifiers, self.keycode)
        } else if self.keycode <= 0x00ff_ffff {
            format!("{}0x{:06x}", modifiers, self.keycode)
        } else {
            "(unknown)".to_string()
        }
    }

    /// 解析文字表示的按鍵。
    pub fn parse(&mut self, repr: &str) -> bool {
        self.parse_bytes(repr.as_bytes())
    }

    fn parse_bytes(&mut self, repr: &[u8]) -> bool {
        self.keycode = 0;
        self.modifier = 0;
        if repr.is_empty() {
            return false;
        }
        if repr.len() == 1 {
            self.keycode = repr[0] as i32;
            return true;
        }
        let mut start = 0;
        while let Some(off) = repr[start..].iter().position(|&b| b == b'+') {
            let found = start + off;
            let token = std::str::from_utf8(&repr[start..found]).unwrap_or("");
            let mask = get_modifier_by_name(token);
            if mask == 0 {
                return false;
            }
            self.modifier |= mask;
            start = found + 1;
        }
        let token = std::str::from_utf8(&repr[start..]).unwrap_or("");
        let keycode = get_keycode_by_name(token);
        if keycode == XK_VOID_SYMBOL {
            return false;
        }
        self.keycode = keycode;
        true
    }
}

impl std::fmt::Display for KeyEvent {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.repr())
    }
}

/// 按鍵序列。
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct KeySequence(Vec<KeyEvent>);

impl KeySequence {
    pub fn new() -> Self {
        Self(Vec::new())
    }

    pub fn from_repr(repr: &str) -> Self {
        let mut seq = Self::new();
        if !seq.parse(repr) {
            seq.clear();
        }
        seq
    }

    /// 可表示為一串文字；其中不產生可打印字符的按鍵以 {鍵名} 標記，組合鍵也用 {組合鍵狀態+鍵名} 標記。
    pub fn repr(&self) -> String {
        let mut result = String::new();
        for ev in &self.0 {
            let k = ev.repr();
            if k.len() == 1 {
                result.push_str(&k);
            } else if is_unescaped_character(ev) {
                result.push((ev.keycode() as u8) as char);
            } else {
                result.push('{');
                result.push_str(&k);
                result.push('}');
            }
        }
        result
    }

    /// 解析按鍵序列描述文字。
    pub fn parse(&mut self, repr: &str) -> bool {
        self.clear();
        let bytes = repr.as_bytes();
        let n = bytes.len();
        let mut i = 0;
        while i < n {
            let (start, len);
            if bytes[i] == b'{' && i + 1 < n {
                start = i + 1;
                let j = match repr[start..].find('}') {
                    Some(off) => start + off,
                    None => return false,
                };
                len = j - start;
                i = j;
            } else {
                start = i;
                len = 1;
            }
            let mut ke = KeyEvent::new();
            if !ke.parse_bytes(&bytes[start..start + len]) {
                return false;
            }
            self.0.push(ke);
            i += 1;
        }
        true
    }

    pub fn push(&mut self, ev: KeyEvent) {
        self.0.push(ev);
    }

    pub fn clear(&mut self) {
        self.0.clear();
    }
}

impl Deref for KeySequence {
    type Target = Vec<KeyEvent>;
    fn deref(&self) -> &Vec<KeyEvent> {
        &self.0
    }
}

impl DerefMut for KeySequence {
    fn deref_mut(&mut self) -> &mut Vec<KeyEvent> {
        &mut self.0
    }
}

impl std::fmt::Display for KeySequence {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.repr())
    }
}

fn is_unescaped_character(ev: &KeyEvent) -> bool {
    let ch = ev.keycode();
    ev.modifier() == 0 && (0x20..=0x7e).contains(&ch) && ch != b'{' as i32 && ch != b'}' as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_and_repr_single_char() {
        let ev = KeyEvent::from_repr("a");
        assert_eq!(ev.keycode(), b'a' as i32);
        assert_eq!(ev.modifier(), 0);
        assert_eq!(ev.repr(), "a");
    }

    #[test]
    fn parse_modifiers() {
        let ev = KeyEvent::from_repr("Control+a");
        assert_eq!(ev.keycode(), b'a' as i32);
        assert!(ev.ctrl());
        assert_eq!(ev.repr(), "Control+a");
    }

    #[test]
    fn parse_named_key() {
        let ev = KeyEvent::from_repr("Return");
        assert_eq!(ev.keycode(), crate::key_table::XK_RETURN);
        assert_eq!(ev.repr(), "Return");
    }

    #[test]
    fn hex_keycode_repr_and_bad_parse() {
        assert_eq!(KeyEvent::with_keycode(0xfffe, 0).repr(), "0xfffe");
        assert_eq!(KeyEvent::with_keycode(0xfffffe, 0).repr(), "0xfffffe");
        assert_eq!(KeyEvent::with_keycode(0x1000000, 0).repr(), "(unknown)");
        // hex representations are produced but not parseable (matches C++)
        let mut ev = KeyEvent::new();
        assert!(!ev.parse("0x12ab"));
    }

    #[test]
    fn modified_key_event_representation() {
        let ev = KeyEvent::with_keycode(
            crate::key_table::XK_LEFT,
            crate::key_table::SHIFT_MASK | crate::key_table::RELEASE_MASK,
        );
        assert_eq!(ev.repr(), "Shift+Release+Left");
    }

    #[test]
    fn parse_key_event_representation() {
        let ev = KeyEvent::from_repr("Shift+Control+Release+A");
        assert_eq!(ev.keycode(), b'A' as i32);
        assert!(ev.shift());
        assert!(!ev.alt());
        assert!(ev.ctrl());
        assert!(ev.release());
    }

    #[test]
    fn key_sequence_round_trip() {
        let seq = KeySequence::from_repr("abc");
        assert_eq!(seq.len(), 3);
        assert_eq!(seq.repr(), "abc");
    }

    #[test]
    fn key_sequence_with_braces() {
        let seq = KeySequence::from_repr("{Control+a}bc");
        assert_eq!(seq.len(), 3);
        assert_eq!(seq.repr(), "{Control+a}bc");
    }
}
