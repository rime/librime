// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/algebra.h and algebra.cc to Rust.

use std::collections::BTreeMap;

use super::calculus::{Calculus, CalculationBox};
use super::spelling::{Spelling, SpellingProperties};
use crate::config::ConfigItem;

/// A script: syllable -> list of spellings.
pub type Script = BTreeMap<String, Vec<Spelling>>;

pub fn script_add_syllable(script: &mut Script, syllable: &str) -> bool {
    if script.contains_key(syllable) {
        return false;
    }
    script.insert(syllable.to_string(), vec![Spelling::new(syllable)]);
    true
}

pub fn script_merge(script: &mut Script, s: &str, sp: &SpellingProperties, v: &[Spelling]) {
    let m = script.entry(s.to_string()).or_default();
    for x in v {
        let mut y = x.clone();
        y.properties.compose(sp);
        match m.iter_mut().find(|e| e.str == x.str) {
            Some(e) => e.properties.update(&y.properties),
            None => m.push(y),
        }
    }
}

/// A sequence of calculations applied in order.
pub struct Projection {
    calculation: Vec<CalculationBox>,
}

impl Default for Projection {
    fn default() -> Self {
        Self::new()
    }
}

impl Projection {
    pub fn new() -> Self {
        Self {
            calculation: Vec::new(),
        }
    }

    pub fn load(&mut self, settings: &[ConfigItem]) -> bool {
        self.calculation.clear();
        let calc = Calculus::new();
        for item in settings {
            let Some(formula) = item.as_str() else {
                self.calculation.clear();
                return false;
            };
            match calc.parse(formula) {
                Some(x) => self.calculation.push(x),
                None => {
                    self.calculation.clear();
                    return false;
                }
            }
        }
        true
    }

    /// Apply to a single spelling string.
    pub fn apply_string(&mut self, value: &mut String) -> bool {
        if value.is_empty() {
            return false;
        }
        let mut s = Spelling::new(value);
        let mut modified = false;
        for x in &mut self.calculation {
            if x.apply(&mut s) {
                modified = true;
            }
        }
        if modified {
            *value = s.str;
        }
        modified
    }

    /// Apply to a whole script.
    pub fn apply_script(&mut self, value: &mut Script) -> bool {
        if value.is_empty() {
            return false;
        }
        let mut modified = false;
        for x in &mut self.calculation {
            let mut temp: Script = Script::new();
            for (k, v) in value.iter() {
                let mut s = Spelling::new(k);
                let applied = x.apply(&mut s);
                if applied {
                    modified = true;
                    if !x.deletion() {
                        script_merge(&mut temp, k, &SpellingProperties::default(), v);
                    }
                    if x.addition() && !s.str.is_empty() {
                        script_merge(&mut temp, &s.str, &s.properties, v);
                    }
                } else {
                    script_merge(&mut temp, k, &SpellingProperties::default(), v);
                }
            }
            *value = temp;
        }
        modified
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::ConfigItem;

    fn formula(s: &str) -> ConfigItem {
        ConfigItem::Scalar(s.to_string())
    }

    #[test]
    fn projection_apply_string() {
        let mut p = Projection::new();
        assert!(p.load(&[
            formula("xlit abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        ]));
        let mut v = "zhong".to_string();
        assert!(p.apply_string(&mut v));
        assert_eq!(v, "ZHONG");
    }

    #[test]
    fn projection_apply_script_derive() {
        let mut p = Projection::new();
        assert!(p.load(&[formula("derive/^([zcs])h/$1/")]));
        let mut script: Script = Script::new();
        script_add_syllable(&mut script, "zh");
        script_add_syllable(&mut script, "ch");
        script_add_syllable(&mut script, "sh");
        script_add_syllable(&mut script, "ba");
        assert!(p.apply_script(&mut script));
        // derive keeps the original and adds the derived form
        assert!(script.contains_key("zh"));
        assert!(script.contains_key("z"));
        assert!(script.contains_key("ba"));
        assert!(!script.contains_key("b"));
    }
}
