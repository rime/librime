// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/translation.h and src/rime/translation.cc to Rust.
//
// Translations are stateful generators of candidates. The C++ code shares them
// via shared_ptr and mutates them in place; the Rust port uses
// `Rc<RefCell<dyn Translation>>` for the same single-threaded, shared, mutable
// semantics.

use std::cell::RefCell;
use std::collections::{BTreeSet, VecDeque};
use std::rc::Rc;

use crate::candidate::{compare as compare_candidates, CandidateArc, CandidateList};

/// Shared, mutably-owned translation (mirrors `an<Translation>`).
pub type TranslationRc = Rc<RefCell<dyn Translation>>;

pub fn new_translation<T: Translation + 'static>(t: T) -> TranslationRc {
    Rc::new(RefCell::new(t))
}

/// A generator of candidates (may yield multiple results).
pub trait Translation {
    fn next(&mut self) -> bool;

    fn peek(&self) -> Option<CandidateArc>;

    /// Negative or zero: provide the next candidate; positive: give up the
    /// chance to other translations.
    fn compare(&self, other: Option<&dyn Translation>, _candidates: &CandidateList) -> i32 {
        match other {
            Some(o) if !o.exhausted() => {
                if self.exhausted() {
                    return 1;
                }
                match (self.peek(), o.peek()) {
                    (Some(ours), Some(theirs)) => {
                        compare_candidates(ours.as_ref(), theirs.as_ref())
                    }
                    _ => 1,
                }
            }
            _ => -1,
        }
    }

    fn exhausted(&self) -> bool;
    fn set_exhausted(&mut self, exhausted: bool);
}

/// A translation that yields a single candidate.
pub struct UniqueTranslation {
    candidate: Option<CandidateArc>,
    exhausted: bool,
}

impl UniqueTranslation {
    pub fn new(candidate: Option<CandidateArc>) -> Self {
        let exhausted = candidate.is_none();
        Self {
            candidate,
            exhausted,
        }
    }
}

impl Translation for UniqueTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        self.exhausted = true;
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            None
        } else {
            self.candidate.clone()
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

/// A translation backed by an ordered list of candidates.
pub struct FifoTranslation {
    candies: Vec<CandidateArc>,
    cursor: usize,
    exhausted: bool,
}

impl FifoTranslation {
    pub fn new() -> Self {
        Self {
            candies: Vec::new(),
            cursor: 0,
            exhausted: true,
        }
    }

    pub fn append(&mut self, candy: CandidateArc) {
        self.candies.push(candy);
        self.exhausted = false;
    }

    pub fn size(&self) -> usize {
        self.candies.len().saturating_sub(self.cursor)
    }
}

impl Default for FifoTranslation {
    fn default() -> Self {
        Self::new()
    }
}

impl Translation for FifoTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        self.cursor += 1;
        if self.cursor >= self.candies.len() {
            self.exhausted = true;
        }
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            None
        } else {
            self.candies.get(self.cursor).cloned()
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

/// Concatenates several translations in order.
pub struct UnionTranslation {
    translations: VecDeque<TranslationRc>,
    exhausted: bool,
}

impl UnionTranslation {
    pub fn new() -> Self {
        Self {
            translations: VecDeque::new(),
            exhausted: true,
        }
    }

    pub fn add(&mut self, t: TranslationRc) {
        if !t.borrow().exhausted() {
            self.translations.push_back(t);
            self.exhausted = false;
        }
    }

    pub fn is_exhausted(&self) -> bool {
        self.exhausted
    }
}

impl Default for UnionTranslation {
    fn default() -> Self {
        Self::new()
    }
}

impl Translation for UnionTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        {
            let front = self.translations.front().expect("non-empty when not exhausted");
            front.borrow_mut().next();
        }
        if self.translations.front().unwrap().borrow().exhausted() {
            self.translations.pop_front();
            if self.translations.is_empty() {
                self.exhausted = true;
            }
        }
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            None
        } else {
            self.translations.front().and_then(|t| t.borrow().peek())
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

pub fn union_translations(
    x: TranslationRc,
    y: TranslationRc,
) -> Option<Rc<RefCell<UnionTranslation>>> {
    let mut z = UnionTranslation::new();
    z.add(x);
    z.add(y);
    if z.exhausted {
        None
    } else {
        Some(Rc::new(RefCell::new(z)))
    }
}

/// Merges several translations, always yielding the best next candidate.
pub struct MergedTranslation {
    previous_candidates: CandidateList,
    translations: Vec<TranslationRc>,
    elected: usize,
    exhausted: bool,
}

impl MergedTranslation {
    pub fn new(previous_candidates: CandidateList) -> Self {
        Self {
            previous_candidates,
            translations: Vec::new(),
            elected: 0,
            exhausted: true,
        }
    }

    pub fn add(&mut self, t: TranslationRc) {
        if !t.borrow().exhausted() {
            self.translations.push(t);
            self.elect();
        }
    }

    pub fn size(&self) -> usize {
        self.translations.len()
    }

    fn elect(&mut self) {
        if self.translations.is_empty() {
            self.exhausted = true;
            return;
        }
        let mut k = 0usize;
        loop {
            if k >= self.translations.len() {
                break;
            }
            let cmp = {
                let current = self.translations[k].borrow();
                let next = self.translations.get(k + 1).map(|t| t.borrow());
                let next_ref: Option<&dyn Translation> = next.as_deref();
                current.compare(next_ref, &self.previous_candidates)
            };
            if cmp <= 0 {
                let exhausted = self.translations[k].borrow().exhausted();
                if exhausted {
                    self.translations.remove(k);
                    k = 0;
                    continue;
                }
                break;
            }
            k += 1;
        }
        self.elected = k;
        self.exhausted = k >= self.translations.len();
    }
}

impl Translation for MergedTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        {
            let elected = self.elected;
            self.translations[elected].borrow_mut().next();
        }
        if self.translations[self.elected].borrow().exhausted() {
            self.translations.remove(self.elected);
        }
        self.elect();
        !self.exhausted
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            None
        } else {
            self.translations
                .get(self.elected)
                .and_then(|t| t.borrow().peek())
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

/// Caches the current candidate of an inner translation.
pub struct CacheTranslation {
    translation: TranslationRc,
    cache: Option<CandidateArc>,
    exhausted: bool,
}

impl CacheTranslation {
    pub fn new(translation: TranslationRc) -> Self {
        let exhausted = translation.borrow().exhausted();
        Self {
            translation,
            cache: None,
            exhausted,
        }
    }
}

impl Translation for CacheTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        self.cache = None;
        self.translation.borrow_mut().next();
        if self.translation.borrow().exhausted() {
            self.exhausted = true;
        }
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            None
        } else {
            match &self.cache {
                Some(c) => Some(c.clone()),
                None => self.translation.borrow().peek(),
            }
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

/// Skips candidates whose text has already been seen.
pub struct DistinctTranslation {
    cache: CacheTranslation,
    candidate_set: BTreeSet<String>,
}

impl DistinctTranslation {
    pub fn new(translation: TranslationRc) -> Self {
        Self {
            cache: CacheTranslation::new(translation),
            candidate_set: BTreeSet::new(),
        }
    }

    fn already_has(&self, text: &str) -> bool {
        self.candidate_set.contains(text)
    }
}

impl Translation for DistinctTranslation {
    fn next(&mut self) -> bool {
        if self.cache.exhausted() {
            return false;
        }
        if let Some(c) = self.cache.peek() {
            self.candidate_set.insert(c.text().to_string());
        }
        loop {
            self.cache.next();
            if self.cache.exhausted() {
                break;
            }
            let text = self.cache.peek().map(|c| c.text().to_string());
            if let Some(text) = text {
                if !self.already_has(&text) {
                    break;
                }
            }
        }
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        self.cache.peek()
    }

    fn exhausted(&self) -> bool {
        self.cache.exhausted()
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.cache.set_exhausted(exhausted);
    }
}

/// Prefetches candidates into a queue (base for look-ahead translators).
pub struct PrefetchTranslation {
    translation: TranslationRc,
    cache: VecDeque<CandidateArc>,
    exhausted: bool,
}

impl PrefetchTranslation {
    pub fn new(translation: TranslationRc) -> Self {
        let exhausted = translation.borrow().exhausted();
        Self {
            translation,
            cache: VecDeque::new(),
            exhausted,
        }
    }

    /// Override point: fill the cache (returns true if it added candidates).
    /// The base class never prefetches; subclasses (look-ahead translators)
    /// will override this. Retained for parity with the C++ `Replenish`.
    #[allow(dead_code)]
    #[allow(unused_variables)]
    fn replenish(&mut self) -> bool {
        false
    }
}

impl Translation for PrefetchTranslation {
    fn next(&mut self) -> bool {
        if self.exhausted {
            return false;
        }
        if !self.cache.is_empty() {
            self.cache.pop_front();
        } else {
            self.translation.borrow_mut().next();
        }
        if self.cache.is_empty() && self.translation.borrow().exhausted() {
            self.exhausted = true;
        }
        true
    }

    fn peek(&self) -> Option<CandidateArc> {
        if self.exhausted {
            return None;
        }
        if !self.cache.is_empty() {
            self.cache.front().cloned()
        } else {
            self.translation.borrow().peek()
        }
    }

    fn exhausted(&self) -> bool {
        self.exhausted
    }
    fn set_exhausted(&mut self, exhausted: bool) {
        self.exhausted = exhausted;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::candidate::{Candidate, CandidateArc, SimpleCandidate};

    fn cand(text: &str, quality: f64) -> CandidateArc {
        let mut c = SimpleCandidate::new("x", 0, 1, text, "", "");
        c.set_quality(quality);
        std::sync::Arc::new(c)
    }

    #[test]
    fn fifo_yields_in_order() {
        let mut t = FifoTranslation::new();
        t.append(cand("a", 1.0));
        t.append(cand("b", 2.0));
        let t = new_translation(t);
        assert_eq!(t.borrow().peek().unwrap().text(), "a");
        assert!(t.borrow_mut().next());
        assert_eq!(t.borrow().peek().unwrap().text(), "b");
        assert!(t.borrow_mut().next());
        assert!(t.borrow().exhausted());
    }

    #[test]
    fn merged_prefers_higher_quality() {
        let mut a = FifoTranslation::new();
        a.append(cand("low", 1.0));
        let mut b = FifoTranslation::new();
        b.append(cand("high", 10.0));
        let mut m = MergedTranslation::new(Vec::new());
        m.add(new_translation(a));
        m.add(new_translation(b));
        let m = new_translation(m);
        assert_eq!(m.borrow().peek().unwrap().text(), "high");
    }

    #[test]
    fn distinct_skips_duplicates() {
        let mut f = FifoTranslation::new();
        f.append(cand("x", 1.0));
        f.append(cand("x", 2.0));
        f.append(cand("y", 3.0));
        let d = new_translation(DistinctTranslation::new(new_translation(f)));
        assert_eq!(d.borrow().peek().unwrap().text(), "x");
        assert!(d.borrow_mut().next());
        assert_eq!(d.borrow().peek().unwrap().text(), "y");
    }
}
