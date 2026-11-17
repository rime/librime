// Copyright RIME Developers
// Distributed under the BSD License
//
// Small helpers shared across the engine.

/// Byte-range slice that never panics on invalid UTF-8 boundaries.
/// Positions are byte offsets, mirroring `std::string::substr` in the C++ code.
/// Returns an empty string when the range is invalid or out of bounds.
pub fn substr(s: &str, start: usize, end: usize) -> &str {
    if start > s.len() {
        return "";
    }
    let end = end.min(s.len());
    s.get(start..end).unwrap_or("")
}

/// Remove the first occurrence of `needle` from `s` (mirrors boost::erase_first_copy).
pub fn erase_first(s: &str, needle: &str) -> String {
    match s.find(needle) {
        Some(pos) => format!("{}{}", &s[..pos], &s[pos + needle.len()..]),
        None => s.to_string(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn substr_basic() {
        assert_eq!(substr("hello", 1, 4), "ell");
        assert_eq!(substr("hello", 1, 100), "ello");
        assert_eq!(substr("hello", 10, 12), "");
    }

    #[test]
    fn erase_first_removes_only_first() {
        assert_eq!(erase_first("a\tb\tc", "\t"), "ab\tc");
        assert_eq!(erase_first("abc", "\t"), "abc");
    }
}
