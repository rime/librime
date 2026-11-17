// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/calculus.h and calculus.cc to Rust.

use std::collections::BTreeMap;

use super::regex::Regex;
use super::spelling::{Spelling, SpellingType};

pub const ABBREVIATION_PENALTY: f64 = -0.6931471805599453; // log(0.5)
pub const FUZZY_SPELLING_PENALTY: f64 = -0.6931471805599453; // log(0.5)
pub const CORRECTION_PENALTY: f64 = -4.605170185988091; // log(0.01)

/// A single spelling-algebra operation.
pub trait Calculation {
    fn apply(&mut self, spelling: &mut Spelling) -> bool;
    fn addition(&self) -> bool {
        true
    }
    fn deletion(&self) -> bool {
        true
    }
}

pub type CalculationBox = Box<dyn Calculation>;

/// The parser/registry for spelling-algebra formulas.
#[derive(Default)]
pub struct Calculus;

impl Calculus {
    pub fn new() -> Self {
        Self
    }

    pub fn parse(&self, definition: &str) -> Option<CalculationBox> {
        // The first non-lowercase character is the argument separator.
        let sep = definition.chars().find(|c| !c.is_ascii_lowercase())?;
        let args: Vec<String> = definition.split(sep).map(|s| s.to_string()).collect();
        let token = args.first()?;
        match token.as_str() {
            "xlit" => Transliteration::parse(&args),
            "xform" => Transformation::parse_xform(&args),
            "erase" => Erasion::parse(&args),
            "derive" => Transformation::parse_derive(&args),
            "fuzz" => Transformation::parse_fuzz(&args),
            "abbrev" => Transformation::parse_abbrev(&args),
            "reorder" => Reorder::parse(&args),
            _ => None,
        }
    }
}

/// xlit/zyx/abc/
pub struct Transliteration {
    char_map: BTreeMap<char, char>,
}

impl Transliteration {
    fn parse(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 3 {
            return None;
        }
        let left: Vec<char> = args[1].chars().collect();
        let right: Vec<char> = args[2].chars().collect();
        if left.len() != right.len() {
            return None;
        }
        let char_map: BTreeMap<char, char> = left.into_iter().zip(right).collect();
        Some(Box::new(Transliteration { char_map }))
    }
}

impl Calculation for Transliteration {
    fn apply(&mut self, spelling: &mut Spelling) -> bool {
        if spelling.str.is_empty() {
            return false;
        }
        let mut result = String::with_capacity(spelling.str.len());
        let mut modified = false;
        for c in spelling.str.chars() {
            match self.char_map.get(&c) {
                Some(&r) => {
                    result.push(r);
                    modified = true;
                }
                None => result.push(c),
            }
        }
        if modified {
            spelling.str = result;
        }
        modified
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum TransformKind {
    Xform,
    Derive,
    Fuzz,
    Abbrev,
    Correction,
}

/// xform/x/y/, derive/x/y/[/tag], fuzz/x/y/, abbrev/x/y/.
pub struct Transformation {
    pattern: Regex,
    replacement: String,
    kind: TransformKind,
}

impl Transformation {
    fn new(left: &str, right: &str, kind: TransformKind) -> Option<CalculationBox> {
        if left.is_empty() {
            return None;
        }
        let pattern = Regex::compile(left).ok()?;
        Some(Box::new(Transformation {
            pattern,
            replacement: right.to_string(),
            kind,
        }))
    }

    fn parse_xform(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 3 {
            return None;
        }
        Self::new(&args[1], &args[2], TransformKind::Xform)
    }

    fn parse_derive(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 3 {
            return None;
        }
        let kind = if args.len() > 3 {
            match args[3].as_str() {
                "correction" => TransformKind::Correction,
                "abbrev" => TransformKind::Abbrev,
                "fuzz" => TransformKind::Fuzz,
                _ => TransformKind::Derive,
            }
        } else {
            TransformKind::Derive
        };
        Self::new(&args[1], &args[2], kind)
    }

    fn parse_fuzz(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 3 {
            return None;
        }
        Self::new(&args[1], &args[2], TransformKind::Fuzz)
    }

    fn parse_abbrev(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 3 {
            return None;
        }
        Self::new(&args[1], &args[2], TransformKind::Abbrev)
    }
}

impl Calculation for Transformation {
    fn apply(&mut self, spelling: &mut Spelling) -> bool {
        if spelling.str.is_empty() {
            return false;
        }
        let result = self.pattern.replace_all(&spelling.str, &self.replacement);
        if result == spelling.str {
            return false;
        }
        spelling.str = result;
        match self.kind {
            TransformKind::Fuzz => {
                spelling.properties.type_ = SpellingType::Fuzzy;
                spelling.properties.credibility += FUZZY_SPELLING_PENALTY;
            }
            TransformKind::Abbrev => {
                spelling.properties.type_ = SpellingType::Abbreviation;
                spelling.properties.credibility += ABBREVIATION_PENALTY;
            }
            TransformKind::Correction => {
                spelling.properties.is_correction = true;
                spelling.properties.credibility += CORRECTION_PENALTY;
            }
            _ => {}
        }
        true
    }

    fn deletion(&self) -> bool {
        self.kind == TransformKind::Xform
    }
}

/// erase/x/
pub struct Erasion {
    pattern: Regex,
}

impl Erasion {
    fn parse(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 2 {
            return None;
        }
        let pattern = &args[1];
        if pattern.is_empty() {
            return None;
        }
        let pattern = Regex::compile(pattern).ok()?;
        Some(Box::new(Erasion { pattern }))
    }
}

impl Calculation for Erasion {
    fn apply(&mut self, spelling: &mut Spelling) -> bool {
        if spelling.str.is_empty() {
            return false;
        }
        if !self.pattern.is_full_match(&spelling.str) {
            return false;
        }
        spelling.str.clear();
        true
    }

    fn addition(&self) -> bool {
        false
    }
}

/// reorder/zyx/dedup
pub struct Reorder {
    order: Vec<char>,
    dedup: bool,
}

impl Reorder {
    fn parse(args: &[String]) -> Option<CalculationBox> {
        if args.len() < 2 {
            return None;
        }
        let order = &args[1];
        if order.is_empty() {
            return None;
        }
        let dedup = args.len() > 2 && args[2] == "dedup";
        Some(Box::new(Reorder {
            order: order.chars().collect(),
            dedup,
        }))
    }
}

impl Calculation for Reorder {
    fn apply(&mut self, spelling: &mut Spelling) -> bool {
        if spelling.str.is_empty() {
            return false;
        }
        let chars: Vec<char> = spelling.str.chars().collect();
        let n = chars.len();
        let mut result: Vec<char> = Vec::new();
        let mut modified = false;
        let mut key_count = vec![0usize; self.order.len()];
        let mut group_start: Option<usize> = None;
        let mut last_key_index = 0usize;
        let mut needs_reorder = false;

        let mut i = 0usize;
        while i < n {
            let here = i;
            let c = chars[i];
            i += 1;
            match self.order.iter().position(|&x| x == c) {
                Some(key_index) => {
                    if group_start.is_none() {
                        group_start = Some(here);
                        needs_reorder = false;
                    } else if key_index < last_key_index {
                        needs_reorder = true;
                    }
                    last_key_index = key_index;
                    key_count[key_index] += 1;
                    if self.dedup && key_count[key_index] > 1 {
                        needs_reorder = true;
                    }
                }
                None => {
                    settle_group(
                        &mut result,
                        &chars,
                        self,
                        &mut key_count,
                        &mut group_start,
                        needs_reorder,
                        &mut modified,
                        here,
                    );
                    if modified {
                        result.push(c);
                    }
                }
            }
        }
        settle_group(
            &mut result,
            &chars,
            self,
            &mut key_count,
            &mut group_start,
            needs_reorder,
            &mut modified,
            n,
        );

        if modified {
            spelling.str = result.into_iter().collect();
        }
        modified
    }
}

#[allow(clippy::too_many_arguments)]
fn settle_group(
    result: &mut Vec<char>,
    chars: &[char],
    reorder: &Reorder,
    key_count: &mut [usize],
    group_start: &mut Option<usize>,
    needs_reorder: bool,
    modified: &mut bool,
    group_end: usize,
) {
    let Some(gs) = *group_start else { return };
    if needs_reorder {
        if !*modified {
            *modified = true;
            result.extend_from_slice(&chars[..gs]);
        }
        for (k, key) in reorder.order.iter().enumerate() {
            if key_count[k] > 0 {
                let cnt = if reorder.dedup { 1 } else { key_count[k] };
                for _ in 0..cnt {
                    result.push(*key);
                }
            }
        }
    } else if *modified {
        result.extend_from_slice(&chars[gs..group_end]);
    }
    for kc in key_count.iter_mut() {
        *kc = 0;
    }
    *group_start = None;
}

#[cfg(test)]
mod tests {
    use super::*;

    const COMBO_PINYIN_KEY_ORDER: &str = "SCZHLFGDBKTPIUÜANREO";

    #[test]
    fn transliteration() {
        let calc = Calculus::new();
        let mut c = calc
            .parse("xlit abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ")
            .unwrap();
        let mut s = Spelling::new("abracadabra");
        assert!(c.apply(&mut s));
        assert_eq!(s.str, "ABRACADABRA");
    }

    #[test]
    fn transformation() {
        let calc = Calculus::new();
        let mut c = calc.parse("xform/^([zcs])h(.*)$/$1$2/").unwrap();
        let mut s = Spelling::new("shang");
        assert!(c.apply(&mut s));
        assert_eq!(s.str, "sang");
        s.str = "bang".to_string();
        assert!(!c.apply(&mut s));
    }

    #[test]
    fn erasion() {
        let calc = Calculus::new();
        let mut c = calc.parse("erase/^[czs]h[aoe]ng?$/").unwrap();
        assert!(!c.addition());
        assert!(c.deletion());
        let mut s = Spelling::new("shang");
        assert!(c.apply(&mut s));
        assert_eq!(s.str, "");
        s.str = "bang".to_string();
        assert!(!c.apply(&mut s));
    }

    #[test]
    fn derivation_and_abbreviation() {
        let calc = Calculus::new();
        let mut c = calc.parse("derive/^([zcs])h/$1/").unwrap();
        assert!(c.addition());
        assert!(!c.deletion());
        let mut s = Spelling::new("shang");
        assert!(c.apply(&mut s));
        assert_eq!(s.str, "sang");

        let mut c = calc.parse("abbrev/^([zcs]h).*$/$1/").unwrap();
        let mut s = Spelling::new("shang");
        assert!(c.apply(&mut s));
        assert_eq!(s.str, "sh");
        assert_eq!(s.properties.type_, SpellingType::Abbreviation);
        assert!((s.properties.credibility - 0.5_f64.ln()).abs() < 1e-12);
    }

    #[test]
    fn reorder_inversion() {
        let calc = Calculus::new();
        let formula = format!("reorder {}", COMBO_PINYIN_KEY_ORDER);
        let mut reorder = calc.parse(&formula).unwrap();
        let mut s = Spelling::new("FZ");
        assert!(reorder.apply(&mut s));
        assert_eq!(s.str, "ZF");

        let mut s = Spelling::new("ZFURO");
        assert!(!reorder.apply(&mut s));
        assert_eq!(s.str, "ZFURO");
    }

    #[test]
    fn reorder_dedup() {
        let calc = Calculus::new();
        let formula = format!("reorder {} dedup", COMBO_PINYIN_KEY_ORDER);
        let mut reorder = calc.parse(&formula).unwrap();
        let mut s = Spelling::new("ZZFFUU");
        assert!(reorder.apply(&mut s));
        assert_eq!(s.str, "ZFU");
    }

    #[test]
    fn reorder_non_order_chars() {
        let calc = Calculus::new();
        let mut reorder = calc.parse("reorder zyx").unwrap();
        let mut s = Spelling::new("xzyaxzy");
        assert!(reorder.apply(&mut s));
        assert_eq!(s.str, "zyxazyx");
    }
}
