// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/spelling.h and spelling.cc to Rust.

/// The kind of a spelling.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Default)]
pub enum SpellingType {
    #[default]
    Normal = 0,
    Fuzzy = 1,
    Abbreviation = 2,
    Completion = 3,
    Ambiguous = 4,
    Invalid = 5,
}

impl SpellingType {
    /// The single-character code used by `Script::Dump` ("-ac?!").
    pub fn code(self) -> char {
        match self {
            SpellingType::Normal => '-',
            SpellingType::Fuzzy => 'a',
            SpellingType::Abbreviation => 'c',
            SpellingType::Completion => '?',
            SpellingType::Ambiguous => '!',
            SpellingType::Invalid => '?',
        }
    }
}

/// Attributes of a spelling produced by a calculation.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct SpellingProperties {
    pub type_: SpellingType,
    pub end_pos: usize,
    pub credibility: f64,
    pub tips: String,
    pub is_correction: bool,
}

impl SpellingProperties {
    /// 疊加運算規則, 將運算規則產生的屬性應用到當前拼寫上.
    pub fn compose(&mut self, delta: &SpellingProperties) {
        if delta.type_ > self.type_ {
            self.type_ = delta.type_;
        }
        self.credibility += delta.credibility;
        if delta.is_correction {
            self.is_correction = true;
        }
        if !delta.tips.is_empty() {
            self.tips = delta.tips.clone();
        }
    }

    /// 更新優化拼寫, 將另一個候選拼寫合併到當前記錄.
    pub fn update(&mut self, other: &SpellingProperties) {
        if self.type_ == other.type_ {
            self.is_correction = self.is_correction && other.is_correction;
        } else if other.type_ < self.type_ {
            self.type_ = other.type_;
            self.is_correction = other.is_correction;
        }
        if other.credibility > self.credibility {
            self.credibility = other.credibility;
        }
        self.tips.clear();
    }
}

/// A spelling (the string plus its properties).
#[derive(Debug, Clone, PartialEq)]
pub struct Spelling {
    pub str: String,
    pub properties: SpellingProperties,
}

impl Spelling {
    pub fn new(s: &str) -> Self {
        Self {
            str: s.to_string(),
            properties: SpellingProperties::default(),
        }
    }
}

impl PartialOrd for Spelling {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.str.cmp(&other.str))
    }
}
