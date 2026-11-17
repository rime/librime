// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/key_table.h and src/rime/key_table.cc to Rust.
// The keysym name table lives in key_table_data.rs (generated from the C++ source).

#[path = "key_table_data.rs"]
mod key_table_data;
pub use key_table_data::{KEY_NAMES, KEYS_BY_KEYVAL, MODIFIER_NAMES};

// --- Modifier masks (RimeModifier in the C++ key_table.h) ---

pub const SHIFT_MASK: i32 = 1 << 0;
pub const LOCK_MASK: i32 = 1 << 1;
pub const CONTROL_MASK: i32 = 1 << 2;
pub const MOD1_MASK: i32 = 1 << 3;
pub const ALT_MASK: i32 = MOD1_MASK;
pub const MOD2_MASK: i32 = 1 << 4;
pub const MOD3_MASK: i32 = 1 << 5;
pub const MOD4_MASK: i32 = 1 << 6;
pub const MOD5_MASK: i32 = 1 << 7;
pub const BUTTON1_MASK: i32 = 1 << 8;
pub const BUTTON2_MASK: i32 = 1 << 9;
pub const BUTTON3_MASK: i32 = 1 << 10;
pub const BUTTON4_MASK: i32 = 1 << 11;
pub const BUTTON5_MASK: i32 = 1 << 12;

pub const HANDLED_MASK: i32 = 1 << 24;
pub const FORWARD_MASK: i32 = 1 << 25;
pub const IGNORED_MASK: i32 = FORWARD_MASK;

pub const SUPER_MASK: i32 = 1 << 26;
pub const HYPER_MASK: i32 = 1 << 27;
pub const META_MASK: i32 = 1 << 28;

pub const RELEASE_MASK: i32 = 1 << 30;

pub const MODIFIER_MASK: i32 = 0x5f00_1fff;

// --- Common X11 keysyms ---

pub const XK_VOID_SYMBOL: i32 = 0x00ff_ffff;
pub const XK_SPACE: i32 = 0x20;
pub const XK_BACK_SPACE: i32 = 0xff08;
pub const XK_TAB: i32 = 0xff09;
pub const XK_LINE_FEED: i32 = 0xff0a;
pub const XK_CLEAR: i32 = 0xff0b;
pub const XK_RETURN: i32 = 0xff0d;
pub const XK_ESCAPE: i32 = 0xff1b;
pub const XK_DELETE: i32 = 0xffff;
pub const XK_HOME: i32 = 0xff50;
pub const XK_LEFT: i32 = 0xff51;
pub const XK_UP: i32 = 0xff52;
pub const XK_RIGHT: i32 = 0xff53;
pub const XK_DOWN: i32 = 0xff54;
pub const XK_PAGE_UP: i32 = 0xff55;
pub const XK_PAGE_DOWN: i32 = 0xff56;
pub const XK_END: i32 = 0xff57;

/// 给定 modifier 文字，返回 mask 值；例如 `get_modifier_by_name("Alt") == 1 << 3`。
/// 不认得所给的键名时返回 0。
pub fn get_modifier_by_name(name: &str) -> i32 {
    for (i, n) in MODIFIER_NAMES.iter().enumerate() {
        if *n == Some(name) {
            return 1i32 << i;
        }
    }
    0
}

/// 给一个数值，取得最低的非 0 位所对应的 modifier 文字；
/// 例如 `get_modifier_name(12) == Some("Control")`。
pub fn get_modifier_name(modifier: i32) -> Option<&'static str> {
    if modifier == 0 {
        return None;
    }
    let idx = modifier.trailing_zeros() as usize;
    if idx >= MODIFIER_NAMES.len() {
        return None;
    }
    MODIFIER_NAMES[idx]
}

/// 由键值取得键名；不认得此键时返回 None。
pub fn get_key_name(keycode: i32) -> Option<&'static str> {
    let key = keycode as u32;
    let idx = KEYS_BY_KEYVAL
        .binary_search_by_key(&key, |&(k, _)| k)
        .ok()?;
    // With duplicate keyvals (eg. apostrophe/quoteright both 0x27) return the first.
    let mut first = idx;
    while first > 0 && KEYS_BY_KEYVAL[first - 1].0 == key {
        first -= 1;
    }
    Some(KEY_NAMES[KEYS_BY_KEYVAL[first].1])
}

/// 由键名取得键值；查无此键则返回 XK_VoidSymbol。
pub fn get_keycode_by_name(name: &str) -> i32 {
    for (i, n) in KEY_NAMES.iter().enumerate() {
        if *n == name {
            // Each name index appears exactly once in the keyval table.
            for &(keyval, idx) in KEYS_BY_KEYVAL {
                if idx == i {
                    return keyval as i32;
                }
            }
            return XK_VOID_SYMBOL;
        }
    }
    XK_VOID_SYMBOL
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn modifier_lookup_round_trip() {
        assert_eq!(get_modifier_by_name("Alt"), 1 << 3);
        assert_eq!(get_modifier_by_name("Control"), 1 << 2);
        assert_eq!(get_modifier_by_name("Super"), SUPER_MASK);
        assert_eq!(get_modifier_by_name("Release"), RELEASE_MASK);
        assert_eq!(get_modifier_by_name("nope"), 0);

        assert_eq!(get_modifier_name(12), Some("Control")); // 0b1100
        assert_eq!(get_modifier_name(0), None);
        assert_eq!(get_modifier_name(SUPER_MASK), Some("Super"));
    }

    #[test]
    fn key_lookup_round_trip() {
        assert_eq!(get_key_name(b'a' as i32), Some("a"));
        assert_eq!(get_key_name(XK_RETURN), Some("Return"));
        assert_eq!(get_key_name(XK_VOID_SYMBOL), Some("VoidSymbol"));
        assert_eq!(get_key_name(0x27), Some("apostrophe"));

        assert_eq!(get_keycode_by_name("a"), b'a' as i32);
        assert_eq!(get_keycode_by_name("Return"), XK_RETURN);
        assert_eq!(get_keycode_by_name("space"), 0x20);
        assert_eq!(get_keycode_by_name("nonsense"), XK_VOID_SYMBOL);
    }
}
