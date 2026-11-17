// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/dict/vocabulary.h — the vocabulary data model.

/// Identifier of a syllable in the syllabary.
pub type SyllableId = i32;
/// A sequence of syllable ids.
pub type Code = Vec<SyllableId>;

pub const K_INDEX_CODE_MAX_LENGTH: usize = 3;

/// A dictionary entry in short form (used during compilation).
#[derive(Debug, Clone, PartialEq)]
pub struct ShortDictEntry {
    pub text: String,
    pub code: Code,
    pub weight: f64,
}

/// A dictionary entry.
#[derive(Debug, Clone)]
pub struct DictEntry {
    pub text: String,
    pub comment: String,
    pub preedit: String,
    pub code: Code,
    pub custom_code: String,
    pub weight: f64,
    /// 全碼匹配長度積分.
    pub quality_len: f64,
    pub commit_count: i32,
    pub remaining_code_length: i32,
    pub matching_code_size: i32,
}

impl Default for DictEntry {
    fn default() -> Self {
        Self {
            text: String::new(),
            comment: String::new(),
            preedit: String::new(),
            code: Code::new(),
            custom_code: String::new(),
            weight: 0.0,
            quality_len: 0.0,
            commit_count: 0,
            remaining_code_length: 0,
            matching_code_size: 0,
        }
    }
}

impl DictEntry {
    pub fn to_short(&self) -> ShortDictEntry {
        ShortDictEntry {
            text: self.text.clone(),
            code: self.code.clone(),
            weight: self.weight,
        }
    }

    pub fn is_exact_match(&self) -> bool {
        self.matching_code_size == 0 || self.matching_code_size == self.code.len() as i32
    }

    pub fn is_predictive_match(&self) -> bool {
        self.matching_code_size != 0 && self.matching_code_size < self.code.len() as i32
    }
}

/// Truncate `code` to at most `K_INDEX_CODE_MAX_LENGTH` syllables.
pub fn create_index(code: &Code, index_code: &mut Code) {
    index_code.clear();
    index_code.extend(code.iter().take(K_INDEX_CODE_MAX_LENGTH).copied());
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn create_index_truncates() {
        let code = vec![1, 2, 3, 4, 5];
        let mut idx = Code::new();
        create_index(&code, &mut idx);
        assert_eq!(idx, vec![1, 2, 3]);
    }
}
