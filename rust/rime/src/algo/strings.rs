// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/strings.h and strings.cc to Rust.

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SplitBehavior {
    KeepToken,
    SkipToken,
}

/// Split `str` on any character in `delim`.
pub fn split(str: &str, delim: &str, behavior: SplitBehavior) -> Vec<String> {
    let is_delim = |c: char| delim.contains(c);
    let chars: Vec<char> = str.chars().collect();
    let mut result: Vec<String> = Vec::new();
    let mut i = 0usize;
    if behavior == SplitBehavior::SkipToken {
        while i < chars.len() && is_delim(chars[i]) {
            i += 1;
        }
    }
    let mut start = i;
    while i < chars.len() {
        if is_delim(chars[i]) {
            result.push(chars[start..i].iter().collect());
            i += 1;
            if behavior == SplitBehavior::SkipToken {
                while i < chars.len() && is_delim(chars[i]) {
                    i += 1;
                }
            }
            start = i;
        } else {
            i += 1;
        }
    }
    if behavior == SplitBehavior::KeepToken || start < chars.len() {
        result.push(chars[start..i].iter().collect());
    }
    result
}

/// Split on any delimiter character, keeping empty tokens.
pub fn split_keep(str: &str, delim: &str) -> Vec<String> {
    split(str, delim, SplitBehavior::KeepToken)
}

/// Join items with `delim`.
pub fn join<'a, I: IntoIterator<Item = &'a str>>(items: I, delim: &str) -> String {
    let mut result = String::new();
    for (i, item) in items.into_iter().enumerate() {
        if i > 0 {
            result.push_str(delim);
        }
        result.push_str(item);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn split_keep_basic() {
        assert_eq!(split_keep("a b c", " "), vec!["a", "b", "c"]);
        assert_eq!(split_keep("xform/a/b/", "/"), vec!["xform", "a", "b", ""]);
    }

    #[test]
    fn join_basic() {
        assert_eq!(join(vec!["a", "b", "c"], " "), "a b c");
    }
}
