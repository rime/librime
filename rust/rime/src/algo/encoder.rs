// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/encoder.h and encoder.cc to Rust.

use std::ops::{Deref, DerefMut};

use super::regex::Regex;
use super::strings::join;
use crate::config::Config;

const K_ENCODER_DFS_LIMIT: i32 = 32;
const K_MAX_PHRASE_LENGTH: usize = 32;

/// A sequence of codes, one per syllable/character.
#[derive(Debug, Clone, Default)]
pub struct RawCode(Vec<String>);

impl RawCode {
    pub fn new() -> Self {
        Self(Vec::new())
    }

    pub fn to_string(&self) -> String {
        join(self.0.iter().map(|s| s.as_str()), " ")
    }

    pub fn from_string(code_str: &str) -> Self {
        let mut result = Self::new();
        let mut rest = code_str.trim_start_matches(' ');
        loop {
            let end = rest.find(' ').unwrap_or(rest.len());
            if end == 0 && rest.is_empty() {
                break;
            }
            result.push(rest[..end].to_string());
            if end == rest.len() {
                break;
            }
            rest = rest[end..].trim_start_matches(' ');
        }
        result
    }
}

impl Deref for RawCode {
    type Target = Vec<String>;
    fn deref(&self) -> &Vec<String> {
        &self.0
    }
}

impl DerefMut for RawCode {
    fn deref_mut(&mut self) -> &mut Vec<String> {
        &mut self.0
    }
}

/// Collects encoded entries during dictionary compilation.
pub trait PhraseCollector {
    fn create_entry(&mut self, phrase: &str, code_str: &str, value: &str);
    /// Return a list of alternative codes for the given word.
    fn translate_word(&mut self, word: &str, code: &mut Vec<String>) -> bool;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CodeCoords {
    pub char_index: i32,
    pub code_index: i32,
}

#[derive(Debug, Clone)]
pub struct TableEncodingRule {
    pub min_word_length: usize,
    pub max_word_length: usize,
    pub coords: Vec<CodeCoords>,
}

fn coord_index(c: char, threshold: char, negative_base: char, zero_base: char) -> i32 {
    if c >= threshold {
        (c as i32) - (negative_base as i32) - 1
    } else {
        (c as i32) - (zero_base as i32)
    }
}

/// Rule-based phrase encoder (for shape-based input methods).
pub struct TableEncoder {
    loaded: bool,
    encoding_rules: Vec<TableEncodingRule>,
    exclude_patterns: Vec<Regex>,
    tail_anchor: String,
    max_phrase_length: usize,
}

impl Default for TableEncoder {
    fn default() -> Self {
        Self::new()
    }
}

impl TableEncoder {
    pub fn new() -> Self {
        Self {
            loaded: false,
            encoding_rules: Vec::new(),
            exclude_patterns: Vec::new(),
            tail_anchor: String::new(),
            max_phrase_length: 0,
        }
    }

    pub fn loaded(&self) -> bool {
        self.loaded
    }

    pub fn encoding_rules(&self) -> &[TableEncodingRule] {
        &self.encoding_rules
    }

    pub fn load_settings(&mut self, config: &Config) -> bool {
        self.loaded = false;
        self.max_phrase_length = 0;
        self.encoding_rules.clear();
        self.exclude_patterns.clear();
        self.tail_anchor.clear();

        if let Some(rules) = config.get_list("encoder/rules") {
            for rule_item in rules {
                let Some(map) = rule_item.as_map() else { continue };
                let Some(formula) = map.get("formula").and_then(|v| v.as_str()) else {
                    continue;
                };
                let Some(mut rule) = Self::parse_formula(formula) else {
                    continue;
                };
                let mut ok = true;
                if let Some(value_item) = map.get("length_equal") {
                    match value_item.get_int() {
                        Some(len) => {
                            let len = len as usize;
                            rule.min_word_length = len;
                            rule.max_word_length = len;
                            self.max_phrase_length = self.max_phrase_length.max(len);
                        }
                        None => ok = false,
                    }
                } else if let Some(range_item) = map.get("length_in_range") {
                    ok = match range_item.as_list().and_then(|r| {
                        if r.len() == 2 {
                            Some((r[0].get_int()?, r[1].get_int()?))
                        } else {
                            None
                        }
                    }) {
                        Some((lo, hi)) if lo <= hi => {
                            rule.min_word_length = lo as usize;
                            rule.max_word_length = hi as usize;
                            self.max_phrase_length = self.max_phrase_length.max(hi as usize);
                            true
                        }
                        _ => false,
                    };
                }
                if ok {
                    self.encoding_rules.push(rule);
                }
            }
            if self.max_phrase_length > K_MAX_PHRASE_LENGTH {
                self.max_phrase_length = K_MAX_PHRASE_LENGTH;
            }
        }
        if let Some(excludes) = config.get_list("encoder/exclude_patterns") {
            for item in excludes {
                if let Some(pattern) = item.as_str() {
                    if let Ok(re) = Regex::compile(pattern) {
                        self.exclude_patterns.push(re);
                    }
                }
            }
        }
        if let Some(tail) = config.get_string("encoder/tail_anchor") {
            self.tail_anchor = tail.to_string();
        }

        self.loaded = !self.encoding_rules.is_empty();
        self.loaded
    }

    fn parse_formula(formula: &str) -> Option<TableEncodingRule> {
        let chars: Vec<char> = formula.chars().collect();
        if chars.len() % 2 != 0 {
            return None;
        }
        let mut coords = Vec::new();
        let mut i = 0;
        while i < chars.len() {
            let ci = chars[i];
            let ki = chars[i + 1];
            if !ci.is_ascii_uppercase() || !ki.is_ascii_lowercase() {
                return None;
            }
            coords.push(CodeCoords {
                char_index: coord_index(ci, 'U', 'Z', 'A'),
                code_index: coord_index(ki, 'u', 'z', 'a'),
            });
            i += 2;
        }
        Some(TableEncodingRule {
            min_word_length: 0,
            max_word_length: 0,
            coords,
        })
    }

    pub fn is_code_excluded(&self, code: &str) -> bool {
        self.exclude_patterns.iter().any(|re| re.is_full_match(code))
    }

    pub fn encode(&self, code: &RawCode, result: &mut String) -> bool {
        let num_syllables = code.len() as i32;
        for rule in &self.encoding_rules {
            if num_syllables < rule.min_word_length as i32
                || num_syllables > rule.max_word_length as i32
            {
                continue;
            }
            result.clear();
            let mut previous = CodeCoords { char_index: 0, code_index: 0 };
            let mut encoded = CodeCoords { char_index: 0, code_index: 0 };
            for current in &rule.coords {
                let mut c = *current;
                if c.char_index < 0 {
                    c.char_index += num_syllables;
                }
                if c.char_index >= num_syllables || c.char_index < 0 {
                    continue;
                }
                if current.char_index < 0 && c.char_index < encoded.char_index {
                    continue;
                }
                let start_index = if c.char_index == encoded.char_index {
                    encoded.code_index + 1
                } else {
                    0
                };
                let syllable = &code[c.char_index as usize];
                c.code_index = self.calculate_code_index(syllable, c.code_index, start_index);
                if c.code_index >= syllable.len() as i32 || c.code_index < 0 {
                    continue;
                }
                if (current.char_index < 0 || current.code_index < 0)
                    && c.char_index == encoded.char_index
                    && c.code_index <= encoded.code_index
                    && (current.char_index != previous.char_index
                        || current.code_index != previous.code_index)
                {
                    continue;
                }
                result.push(syllable.as_bytes()[c.code_index as usize] as char);
                previous = *current;
                encoded = c;
            }
            if result.is_empty() {
                continue;
            }
            return true;
        }
        false
    }

    fn calculate_code_index(&self, code: &str, index: i32, start: i32) -> i32 {
        let bytes = code.as_bytes();
        let n = bytes.len() as i32;
        let is_anchor = |b: u8| self.tail_anchor.contains(b as char);
        let mut k: i32;
        if index < 0 {
            k = n - 1;
            let mut tail = None;
            let from = (start + 1).max(0) as usize;
            for i in from..bytes.len() {
                if is_anchor(bytes[i]) {
                    tail = Some(i as i32);
                    break;
                }
            }
            if let Some(t) = tail {
                k = t - 1;
            }
            let mut idx = index + 1;
            while idx < 0 {
                loop {
                    k -= 1;
                    if k < 0 || !is_anchor(bytes[k as usize]) {
                        break;
                    }
                }
                idx += 1;
            }
        } else {
            k = 0;
            let mut idx = index;
            while idx > 0 {
                loop {
                    k += 1;
                    if k >= n || !is_anchor(bytes[k as usize]) {
                        break;
                    }
                }
                idx -= 1;
            }
        }
        k
    }

    pub fn encode_phrase(
        &mut self,
        phrase: &str,
        value: &str,
        collector: &mut dyn PhraseCollector,
    ) -> bool {
        if phrase.chars().count() > self.max_phrase_length {
            return false;
        }
        let mut code = RawCode::new();
        let mut limit = K_ENCODER_DFS_LIMIT;
        self.dfs_encode(phrase, value, 0, &mut code, &mut limit, collector)
    }

    fn dfs_encode(
        &mut self,
        phrase: &str,
        value: &str,
        start_pos: usize,
        code: &mut RawCode,
        limit: &mut i32,
        collector: &mut dyn PhraseCollector,
    ) -> bool {
        if start_pos == phrase.len() {
            *limit -= 1;
            let mut encoded = String::new();
            if self.encode(code, &mut encoded) {
                collector.create_entry(phrase, &encoded, value);
                return true;
            }
            return false;
        }
        let word: String = phrase[start_pos..].chars().take(1).collect();
        let word_len = word.len();
        let mut ret = false;
        let mut translations = Vec::new();
        if collector.translate_word(&word, &mut translations) {
            for x in &translations {
                if self.is_code_excluded(x) {
                    continue;
                }
                code.push(x.clone());
                let ok = self.dfs_encode(phrase, value, start_pos + word_len, code, limit, collector);
                ret = ret || ok;
                code.pop();
                if *limit <= 0 {
                    return ret;
                }
            }
        }
        ret
    }
}

/// Syllable-based phrase encoder (for phonetic input methods).
pub struct ScriptEncoder;

impl ScriptEncoder {
    pub fn new() -> Self {
        Self
    }

    pub fn encode_phrase(
        &mut self,
        phrase: &str,
        value: &str,
        collector: &mut dyn PhraseCollector,
    ) -> bool {
        if phrase.chars().count() > K_MAX_PHRASE_LENGTH {
            return false;
        }
        let mut code = RawCode::new();
        let mut limit = K_ENCODER_DFS_LIMIT;
        self.dfs_encode(phrase, value, 0, &mut code, &mut limit, collector)
    }

    fn dfs_encode(
        &mut self,
        phrase: &str,
        value: &str,
        start_pos: usize,
        code: &mut RawCode,
        limit: &mut i32,
        collector: &mut dyn PhraseCollector,
    ) -> bool {
        if start_pos == phrase.len() {
            *limit -= 1;
            collector.create_entry(phrase, &code.to_string(), value);
            return true;
        }
        // try progressively shorter suffixes (longest word first)
        let mut ends: Vec<usize> = phrase[start_pos..]
            .char_indices()
            .map(|(i, c)| start_pos + i + c.len_utf8())
            .collect();
        ends.sort_by(|a, b| b.cmp(a));
        let mut ret = false;
        for end in ends {
            let word = &phrase[start_pos..end];
            let mut translations = Vec::new();
            if collector.translate_word(word, &mut translations) {
                for x in &translations {
                    code.push(x.clone());
                    let ok = self.dfs_encode(phrase, value, end, code, limit, collector);
                    ret = ret || ok;
                    code.pop();
                    if *limit <= 0 {
                        return ret;
                    }
                }
            }
        }
        ret
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::ConfigItem;
    use std::collections::BTreeMap;

    fn scalar(s: &str) -> ConfigItem {
        ConfigItem::Scalar(s.to_string())
    }

    fn config_with_rules(rules: Vec<BTreeMap<String, ConfigItem>>) -> Config {
        let rule_items: Vec<ConfigItem> = rules.into_iter().map(ConfigItem::Map).collect();
        let encoder_map = BTreeMap::from([
            ("rules".to_string(), ConfigItem::List(rule_items)),
            ("tail_anchor".to_string(), scalar("'")),
        ]);
        Config::new(ConfigItem::Map(BTreeMap::from([(
            "encoder".to_string(),
            ConfigItem::Map(encoder_map),
        )])))
    }

    #[test]
    fn load_settings_parses_formula() {
        let mut config = config_with_rules(vec![
            BTreeMap::from([
                ("length_equal".to_string(), scalar("2")),
                ("formula".to_string(), scalar("AaAzBaBz")),
            ]),
            BTreeMap::from([
                ("length_equal".to_string(), scalar("3")),
                ("formula".to_string(), scalar("AaBaCaBz")),
            ]),
        ]);
        config.root = {
            // add length_in_range rule
            let mut root = match config.root {
                ConfigItem::Map(m) => m,
                _ => BTreeMap::new(),
            };
            if let Some(ConfigItem::Map(enc)) = root.get_mut("encoder") {
                if let Some(ConfigItem::List(rules)) = enc.get_mut("rules") {
                    rules.push(ConfigItem::Map(BTreeMap::from([
                        (
                            "length_in_range".to_string(),
                            ConfigItem::List(vec![scalar("4"), scalar("9")]),
                        ),
                        ("formula".to_string(), scalar("AaBaCaZz")),
                    ])));
                }
            }
            ConfigItem::Map(root)
        };
        let config = Config::new(config.root);
        let mut encoder = TableEncoder::new();
        assert!(encoder.load_settings(&config));
        assert_eq!(encoder.encoding_rules().len(), 3);
        assert_eq!(encoder.encoding_rules()[0].min_word_length, 2);
        assert_eq!(encoder.encoding_rules()[0].max_word_length, 2);
        assert_eq!(encoder.encoding_rules()[0].coords.len(), 4);
        assert_eq!(encoder.encoding_rules()[0].coords[0].char_index, 0);
        assert_eq!(encoder.encoding_rules()[0].coords[1].code_index, -1);
    }

    #[test]
    fn raw_code_roundtrip() {
        let c = RawCode::from_string("abc def");
        assert_eq!(c.len(), 2);
        assert_eq!(c.to_string(), "abc def");
    }

    #[test]
    fn encode_basic() {
        let config = config_with_rules(vec![BTreeMap::from([
            ("length_equal".to_string(), scalar("2")),
            ("formula".to_string(), scalar("AaAbBaBb")),
        ])]);
        let mut encoder = TableEncoder::new();
        assert!(encoder.load_settings(&config));
        let c0 = RawCode::from_string("abc def");
        let mut result = String::new();
        assert!(encoder.encode(&c0, &mut result));
        assert_eq!(result, "abde");
    }
}
