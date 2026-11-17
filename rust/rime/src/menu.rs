// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/menu.h and src/rime/menu.cc to Rust.

use std::cell::RefCell;
use std::rc::Rc;

use crate::candidate::{CandidateArc, CandidateList};
use crate::filter::Filter;
use crate::translation::{MergedTranslation, TranslationRc};

/// A page of candidates.
#[derive(Clone, Default)]
pub struct Page {
    pub page_size: usize,
    pub page_no: usize,
    pub is_last_page: bool,
    pub candidates: CandidateList,
}

/// A menu gathers candidates from translations and optionally filters them.
pub struct Menu {
    merged: Rc<RefCell<MergedTranslation>>,
    result: TranslationRc,
    candidates: CandidateList,
}

impl Menu {
    pub fn new() -> Self {
        let merged = Rc::new(RefCell::new(MergedTranslation::new(Vec::new())));
        let result: TranslationRc = merged.clone();
        Self {
            merged,
            result,
            candidates: Vec::new(),
        }
    }

    pub fn add_translation(&mut self, translation: TranslationRc) {
        self.merged.borrow_mut().add(translation);
    }

    pub fn add_filter(&mut self, filter: &mut dyn Filter) {
        let result = filter.apply(self.result.clone(), &mut self.candidates);
        self.result = result;
    }

    pub fn prepare(&mut self, requested: usize) -> usize {
        while self.candidates.len() < requested && !self.result.borrow().exhausted() {
            let cand = self.result.borrow().peek();
            if let Some(c) = cand {
                self.candidates.push(c);
            }
            self.result.borrow_mut().next();
        }
        self.candidates.len()
    }

    pub fn create_page(&mut self, page_size: usize, page_no: usize) -> Option<Page> {
        let start_pos = page_size * page_no;
        let mut end_pos = start_pos + page_size;
        if end_pos > self.candidates.len() {
            if self.result.borrow().exhausted() {
                end_pos = self.candidates.len();
            } else {
                end_pos = self.prepare(end_pos);
            }
            if start_pos >= end_pos {
                return None;
            }
            end_pos = (start_pos + page_size).min(end_pos);
        }
        let is_last_page = self.result.borrow().exhausted() && end_pos == self.candidates.len();
        Some(Page {
            page_size,
            page_no,
            is_last_page,
            candidates: self.candidates[start_pos..end_pos].to_vec(),
        })
    }

    pub fn get_candidate_at(&mut self, index: usize) -> Option<CandidateArc> {
        if index >= self.candidates.len() && index >= self.prepare(index + 1) {
            return None;
        }
        self.candidates.get(index).cloned()
    }

    /// The number of candidates currently obtained (not the total available).
    pub fn candidate_count(&self) -> usize {
        self.candidates.len()
    }

    pub fn empty(&self) -> bool {
        self.candidates.is_empty() && self.result.borrow().exhausted()
    }
}

impl Default for Menu {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::candidate::{CandidateArc, SimpleCandidate};
    use crate::translation::{new_translation, FifoTranslation};

    fn cand(text: &str) -> CandidateArc {
        std::sync::Arc::new(SimpleCandidate::new("x", 0, 1, text, "", ""))
    }

    #[test]
    fn menu_prepares_and_pages() {
        let mut t = FifoTranslation::new();
        for i in 0..5 {
            t.append(cand(&format!("c{i}")));
        }
        let mut menu = Menu::new();
        menu.add_translation(new_translation(t));

        let page = menu.create_page(3, 0).unwrap();
        assert_eq!(page.candidates.len(), 3);
        assert!(!page.is_last_page);

        let page = menu.create_page(3, 1).unwrap();
        assert_eq!(page.candidates.len(), 2);
        assert!(page.is_last_page);
    }

    #[test]
    fn empty_menu() {
        let mut menu = Menu::new();
        assert!(menu.empty());
        assert_eq!(menu.prepare(10), 0);
    }
}
