// Copyright RIME Developers
// Distributed under the BSD License
//
// A simplified, in-memory dictionary table. It replaces librime's memory-mapped
// binary table (head/trunk/tail index over a string table) with a trie keyed by
// syllable-id sequences, providing the same lookup semantics needed at runtime:
// exact and predictive (prefix) queries over multi-syllable codes.

use std::collections::HashMap;

use super::vocabulary::{DictEntry, SyllableId};

struct TrieNode {
    entries: Vec<DictEntry>,
    children: HashMap<SyllableId, TrieNode>,
}

impl Default for TrieNode {
    fn default() -> Self {
        Self {
            entries: Vec::new(),
            children: HashMap::new(),
        }
    }
}

/// An in-memory dictionary table.
#[derive(Default)]
pub struct Table {
    syllabary: Vec<String>,
    syllable_to_id: HashMap<String, SyllableId>,
    root: TrieNode,
}

impl Table {
    pub fn new() -> Self {
        Self::default()
    }

    /// Load entries from a `.dict.yaml` source (tab-separated `text\tcode[\tweight]`
    /// after the `...` marker). Weights like `99.93%` are parsed as plain floats.
    pub fn from_dict_yaml(input: &str) -> Self {
        let mut table = Table::new();
        let mut in_entries = false;
        for line in input.lines() {
            let line = line.trim_end();
            if line == "..." {
                in_entries = true;
                continue;
            }
            if !in_entries || line.is_empty() || line.starts_with('#') {
                continue;
            }
            let fields: Vec<&str> = line.split('\t').collect();
            if fields.len() < 2 {
                continue;
            }
            let text = fields[0].trim();
            if text.is_empty() {
                continue;
            }
            let code: Vec<&str> = fields[1].split_whitespace().collect();
            if code.is_empty() {
                continue;
            }
            let weight = fields.get(2).and_then(|w| parse_weight(w)).unwrap_or(0.0);
            table.add_entry(&code, text, weight);
        }
        table
    }

    /// Register a syllable and return its id (reusing an existing id).
    pub fn add_syllable(&mut self, syllable: &str) -> SyllableId {
        if let Some(&id) = self.syllable_to_id.get(syllable) {
            return id;
        }
        let id = self.syllabary.len() as SyllableId;
        self.syllabary.push(syllable.to_string());
        self.syllable_to_id.insert(syllable.to_string(), id);
        id
    }

    pub fn get_syllable_id(&self, syllable: &str) -> Option<SyllableId> {
        self.syllable_to_id.get(syllable).copied()
    }

    pub fn get_syllable_by_id(&self, id: SyllableId) -> Option<&str> {
        self.syllabary.get(id as usize).map(|s| s.as_str())
    }

    pub fn syllabary(&self) -> &[String] {
        &self.syllabary
    }

    /// Add an entry keyed by a sequence of syllable strings.
    pub fn add_entry(&mut self, code: &[&str], text: &str, weight: f64) {
        let ids: Vec<SyllableId> = code.iter().map(|s| self.add_syllable(s)).collect();
        let mut entry = DictEntry {
            text: text.to_string(),
            code: ids.clone(),
            weight,
            ..Default::default()
        };
        entry.matching_code_size = ids.len() as i32;
        self.add_entry_ids(&ids, entry);
    }

    fn add_entry_ids(&mut self, code: &[SyllableId], entry: DictEntry) {
        let mut node = &mut self.root;
        for &sid in code {
            node = node.children.entry(sid).or_default();
        }
        node.entries.push(entry);
    }

    /// Look up entries matching `code`.
    /// With `predictive`, also returns entries whose code extends `code`.
    pub fn lookup(&self, code: &[SyllableId], predictive: bool) -> Vec<DictEntry> {
        let mut node = &self.root;
        for &sid in code {
            match node.children.get(&sid) {
                Some(child) => node = child,
                None => return Vec::new(),
            }
        }
        let mut result = Vec::new();
        if predictive {
            collect_entries(node, &mut result);
        } else {
            result.extend(node.entries.iter().cloned());
        }
        result
    }

    /// Convenience: look up by syllable strings.
    pub fn lookup_str(&self, code: &[&str], predictive: bool) -> Vec<DictEntry> {
        let mut ids = Vec::with_capacity(code.len());
        for s in code {
            match self.get_syllable_id(s) {
                Some(id) => ids.push(id),
                None => return Vec::new(),
            }
        }
        self.lookup(&ids, predictive)
    }
}

fn collect_entries(node: &TrieNode, out: &mut Vec<DictEntry>) {
    out.extend(node.entries.iter().cloned());
    for child in node.children.values() {
        collect_entries(child, out);
    }
}

fn parse_weight(w: &str) -> Option<f64> {
    w.trim().trim_end_matches('%').parse::<f64>().ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn build_table() -> Table {
        let mut t = Table::new();
        t.add_entry(&["ni"], "你", 1.0);
        t.add_entry(&["hao"], "好", 1.0);
        t.add_entry(&["hao"], "号", 0.5);
        t.add_entry(&["zhong", "guo"], "中国", 2.0);
        t.add_entry(&["ni", "hao"], "你好", 1.5);
        t
    }

    #[test]
    fn exact_lookup() {
        let t = build_table();
        let r = t.lookup_str(&["hao"], false);
        assert_eq!(r.len(), 2);
        assert_eq!(r[0].text, "好");
    }

    #[test]
    fn phrase_lookup() {
        let t = build_table();
        let r = t.lookup_str(&["ni", "hao"], false);
        assert_eq!(r.len(), 1);
        assert_eq!(r[0].text, "你好");
    }

    #[test]
    fn predictive_lookup() {
        let t = build_table();
        // "zhong" predicts "中国" and "zhong" itself (if present); here just "中国"
        let r = t.lookup_str(&["zhong"], true);
        assert_eq!(r.len(), 1);
        assert_eq!(r[0].text, "中国");
    }

    #[test]
    fn syllable_ids() {
        let t = build_table();
        assert_eq!(t.get_syllable_id("ni"), Some(0));
        assert_eq!(t.get_syllable_by_id(0), Some("ni"));
    }
}
