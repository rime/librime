// Copyright RIME Developers
// Distributed under the BSD License
//
// A simplified Prism: a trie over syllable spellings, providing common-prefix
// search. In the original librime this is a double-array trie (darts) plus a
// spelling map supporting the spelling algebra; here, in the no-spelling-algebra
// case, each spelling maps directly to one syllable id.

use std::collections::HashMap;

use super::vocabulary::SyllableId;

#[derive(Default)]
struct PrismNode {
    syllable_id: Option<SyllableId>,
    children: HashMap<char, PrismNode>,
}

/// A trie over syllable spellings.
pub struct Prism {
    root: PrismNode,
}

impl Prism {
    pub fn from_syllabary(syllabary: &[String]) -> Self {
        let mut root = PrismNode::default();
        for (id, s) in syllabary.iter().enumerate() {
            let mut node = &mut root;
            for c in s.chars() {
                node = node.children.entry(c).or_default();
            }
            node.syllable_id = Some(id as SyllableId);
        }
        Self { root }
    }

    /// Whether `key` is exactly a syllable spelling.
    pub fn has_key(&self, key: &str) -> bool {
        let mut node = &self.root;
        for c in key.chars() {
            match node.children.get(&c) {
                Some(child) => node = child,
                None => return false,
            }
        }
        node.syllable_id.is_some()
    }

    /// All syllable spellings that are prefixes of `input`, as (syllable_id, byte_length).
    pub fn common_prefix_search(&self, input: &str) -> Vec<(SyllableId, usize)> {
        let mut result = Vec::new();
        let mut node = &self.root;
        for (byte_offset, c) in input.char_indices() {
            match node.children.get(&c) {
                Some(child) => {
                    node = child;
                    if let Some(id) = node.syllable_id {
                        result.push((id, byte_offset + c.len_utf8()));
                    }
                }
                None => break,
            }
        }
        result
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn common_prefix_search() {
        let prism = Prism::from_syllabary(&["ni".to_string(), "hao".to_string(), "nihao".to_string()]);
        let m = prism.common_prefix_search("nihao");
        // "ni" (len 2), "nihao" (len 5)
        assert_eq!(m.len(), 2);
        assert_eq!(m[0].1, 2);
        assert_eq!(m[1].1, 5);
        assert!(prism.has_key("ni"));
        assert!(!prism.has_key("niha"));
    }
}
