// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/context.h and src/rime/context.cc to Rust.

use std::collections::BTreeMap;
use std::rc::Rc;
use std::cell::RefCell;

use crate::candidate::CandidateArc;
use crate::commit_history::CommitHistory;
use crate::composition::{CandidatePreview, Composition, Preedit};
use crate::key_event::KeyEvent;
use crate::segmentation::SegmentStatus;

/// Shared, mutably-owned context (used by the engine and components).
pub type ContextRc = Rc<RefCell<Context>>;

const SELECTED_BEFORE_EDITING: &str = "selected_before_editing";
const CARET_SYMBOL: &str = "\u{2038}"; // U+2038 ‸ CARET

// Fire a notifier whose handlers receive `&mut Context` (plus optional args).
macro_rules! fire {
    ($self:ident, $field:ident) => {{
        let mut handlers = std::mem::take(&mut $self.$field);
        for h in handlers.iter_mut() {
            h(&mut *$self);
        }
        $self.$field = handlers;
    }};
    ($self:ident, $field:ident, $arg:expr) => {{
        let mut handlers = std::mem::take(&mut $self.$field);
        for h in handlers.iter_mut() {
            h(&mut *$self, $arg);
        }
        $self.$field = handlers;
    }};
}

/// Per-session input state.
pub struct Context {
    input: String,
    caret_pos: usize,
    composition: Composition,
    commit_history: CommitHistory,
    options: BTreeMap<String, bool>,
    properties: BTreeMap<String, String>,

    commit_notifier: Vec<Box<dyn FnMut(&mut Context)>>,
    select_notifier: Vec<Box<dyn FnMut(&mut Context)>>,
    update_notifier: Vec<Box<dyn FnMut(&mut Context)>>,
    delete_notifier: Vec<Box<dyn FnMut(&mut Context)>>,
    abort_notifier: Vec<Box<dyn FnMut(&mut Context)>>,
    option_update_notifier: Vec<Box<dyn for<'a> FnMut(&mut Context, &'a str)>>,
    property_update_notifier: Vec<Box<dyn for<'a> FnMut(&mut Context, &'a str)>>,
    unhandled_key_notifier: Vec<Box<dyn for<'a> FnMut(&mut Context, &'a KeyEvent)>>,
}

impl Default for Context {
    fn default() -> Self {
        Self::new()
    }
}

impl Context {
    pub fn new() -> Self {
        Self {
            input: String::new(),
            caret_pos: 0,
            composition: Composition::new(),
            commit_history: CommitHistory::new(),
            options: BTreeMap::new(),
            properties: BTreeMap::new(),
            commit_notifier: Vec::new(),
            select_notifier: Vec::new(),
            update_notifier: Vec::new(),
            delete_notifier: Vec::new(),
            abort_notifier: Vec::new(),
            option_update_notifier: Vec::new(),
            property_update_notifier: Vec::new(),
            unhandled_key_notifier: Vec::new(),
        }
    }

    // --- notifier registration ---

    pub fn on_commit(&mut self, f: Box<dyn FnMut(&mut Context)>) {
        self.commit_notifier.push(f);
    }
    pub fn on_select(&mut self, f: Box<dyn FnMut(&mut Context)>) {
        self.select_notifier.push(f);
    }
    pub fn on_update(&mut self, f: Box<dyn FnMut(&mut Context)>) {
        self.update_notifier.push(f);
    }
    pub fn on_delete(&mut self, f: Box<dyn FnMut(&mut Context)>) {
        self.delete_notifier.push(f);
    }
    pub fn on_abort(&mut self, f: Box<dyn FnMut(&mut Context)>) {
        self.abort_notifier.push(f);
    }
    pub fn on_option_update(&mut self, f: Box<dyn for<'a> FnMut(&mut Context, &'a str)>) {
        self.option_update_notifier.push(f);
    }
    pub fn on_property_update(&mut self, f: Box<dyn for<'a> FnMut(&mut Context, &'a str)>) {
        self.property_update_notifier.push(f);
    }
    pub fn on_unhandled_key(&mut self, f: Box<dyn for<'a> FnMut(&mut Context, &'a KeyEvent)>) {
        self.unhandled_key_notifier.push(f);
    }

    /// Record the current composition into the commit history.
    pub fn record_commit(&mut self) {
        self.commit_history.push_composition(&self.composition, &self.input);
    }

    /// Notify interested parties of an unhandled key event.
    pub fn notify_unhandled_key(&mut self, key_event: &KeyEvent) {
        fire!(self, unhandled_key_notifier, key_event);
    }

    // --- queries ---

    pub fn commit(&mut self) -> bool {
        if !self.is_composing() {
            return false;
        }
        fire!(self, commit_notifier);
        self.clear();
        true
    }

    pub fn get_commit_text(&self) -> String {
        if self.get_option("dumb") {
            return String::new();
        }
        self.composition.get_commit_text()
    }

    pub fn get_script_text(&self) -> String {
        self.composition.get_script_text(true)
    }

    pub fn get_preedit(&self) -> Preedit {
        let soft_cursor = self.get_soft_cursor();
        self.composition.get_preedit(&self.input, self.caret_pos, &soft_cursor)
    }

    pub fn is_composing(&self) -> bool {
        !self.input.is_empty() || !self.composition.is_empty()
    }

    pub fn has_menu(&self) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        match self.composition.last() {
            Some(seg) => seg.menu.as_ref().map(|m| !m.borrow().empty()).unwrap_or(false),
            None => false,
        }
    }

    pub fn get_selected_candidate(&self) -> Option<CandidateArc> {
        self.composition.last().and_then(|s| s.get_selected_candidate())
    }

    pub fn get_candidate_preview(&self) -> CandidatePreview {
        self.composition.get_candidate_preview(&self.input)
    }

    // --- input editing ---

    pub fn push_input(&mut self, ch: char) -> bool {
        if self.caret_pos >= self.input.len() {
            self.input.push(ch);
            self.caret_pos = self.input.len();
        } else {
            self.input.insert(self.caret_pos, ch);
            self.caret_pos += ch.len_utf8();
        }
        fire!(self, update_notifier);
        true
    }

    pub fn push_input_str(&mut self, s: &str) -> bool {
        if self.caret_pos >= self.input.len() {
            self.input.push_str(s);
            self.caret_pos = self.input.len();
        } else {
            self.input.insert_str(self.caret_pos, s);
            self.caret_pos += s.len();
        }
        fire!(self, update_notifier);
        true
    }

    pub fn pop_input(&mut self, len: usize) -> bool {
        if self.caret_pos < len {
            return false;
        }
        self.caret_pos -= len;
        self.input.replace_range(self.caret_pos..self.caret_pos + len, "");
        fire!(self, update_notifier);
        true
    }

    pub fn delete_input(&mut self, len: usize) -> bool {
        if self.caret_pos + len > self.input.len() {
            return false;
        }
        self.input.replace_range(self.caret_pos..self.caret_pos + len, "");
        fire!(self, update_notifier);
        true
    }

    pub fn clear(&mut self) {
        self.input.clear();
        self.caret_pos = 0;
        self.composition.clear();
        fire!(self, update_notifier);
    }

    /// Clear and notify abort.
    pub fn abort_composition(&mut self) {
        self.clear();
        fire!(self, abort_notifier);
    }

    // --- selection ---

    /// Returns false if there is no candidate at `index`.
    pub fn select(&mut self, index: usize) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        {
            let seg = self.composition.last_mut().unwrap();
            if seg.get_candidate_at(index).is_none() {
                return false;
            }
            seg.selected_index = index;
            seg.status = SegmentStatus::Selected;
        }
        fire!(self, select_notifier);
        true
    }

    /// Returns false if the selected index has not changed.
    pub fn highlight(&mut self, index: usize) -> bool {
        if self.composition.is_empty()
            || self.composition.last().map(|s| s.menu.is_none()).unwrap_or(true)
        {
            return false;
        }
        let (previous_index, new_index);
        {
            let seg = self.composition.last_mut().unwrap();
            let candidate_count = seg
                .menu
                .as_ref()
                .map(|m| m.borrow_mut().prepare(index + 1))
                .unwrap_or(0);
            let new = if candidate_count > 0 {
                (candidate_count - 1).min(index)
            } else {
                0
            };
            previous_index = seg.selected_index;
            new_index = new;
            seg.selected_index = new;
        }
        if previous_index == new_index {
            return false;
        }
        fire!(self, update_notifier);
        true
    }

    pub fn delete_candidate(&mut self, index: usize) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        {
            let seg = self.composition.last_mut().unwrap();
            seg.selected_index = index;
        }
        fire!(self, delete_notifier);
        true
    }

    pub fn delete_current_selection(&mut self) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        let index = self.composition.last().map(|s| s.selected_index).unwrap_or(0);
        self.delete_candidate(index)
    }

    /// Returns false if there's no candidate for the current segment.
    pub fn confirm_current_selection(&mut self) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        {
            let seg = self.composition.last_mut().unwrap();
            seg.status = SegmentStatus::Selected;
            let has_candidate = seg.get_selected_candidate().is_some();
            if !has_candidate && seg.end == seg.start {
                return false;
            }
        }
        fire!(self, select_notifier);
        true
    }

    pub fn begin_editing(&mut self) {
        for seg in self.composition.iter_mut().rev() {
            if seg.status > SegmentStatus::Selected {
                return;
            }
            if seg.status == SegmentStatus::Selected {
                seg.tags.insert(SELECTED_BEFORE_EDITING.to_string());
                return;
            }
        }
    }

    pub fn confirm_previous_selection(&mut self) -> bool {
        self.begin_editing();
        false
    }

    pub fn reopen_previous_segment(&mut self) -> bool {
        if self.composition.trim() {
            let caret = self.caret_pos;
            if !self.composition.is_empty()
                && self.composition.last().map(|s| s.status >= SegmentStatus::Selected).unwrap_or(false)
            {
                if let Some(seg) = self.composition.last_mut() {
                    seg.reopen(caret);
                }
            }
            fire!(self, update_notifier);
            return true;
        }
        false
    }

    pub fn clear_previous_segment(&mut self) -> bool {
        if self.composition.is_empty() {
            return false;
        }
        let where_ = self.composition.last().map(|s| s.start).unwrap_or(0);
        if where_ >= self.input.len() {
            return false;
        }
        self.set_input(&self.input[..where_].to_string());
        true
    }

    pub fn reopen_previous_selection(&mut self) -> bool {
        let caret = self.caret_pos;
        // find the last selected segment
        let mut selected = None;
        for (i, seg) in self.composition.iter().enumerate().rev() {
            if seg.status > SegmentStatus::Selected {
                return false;
            }
            if seg.status == SegmentStatus::Selected {
                if seg.has_tag(SELECTED_BEFORE_EDITING) {
                    return false;
                }
                selected = Some(i);
                break;
            }
        }
        let Some(i) = selected else {
            return false;
        };
        // pop segments after the selected one
        while self.composition.len() - 1 > i {
            self.composition.pop();
        }
        if let Some(seg) = self.composition.last_mut() {
            seg.reopen(caret);
        }
        fire!(self, update_notifier);
        true
    }

    pub fn clear_non_confirmed_composition(&mut self) -> bool {
        let mut reverted = false;
        while !self.composition.is_empty()
            && self.composition.last().map(|s| s.status < SegmentStatus::Selected).unwrap_or(false)
        {
            self.composition.pop();
            reverted = true;
        }
        if reverted {
            self.composition.forward();
        }
        reverted
    }

    pub fn refresh_non_confirmed_composition(&mut self) -> bool {
        if self.clear_non_confirmed_composition() {
            fire!(self, update_notifier);
            return true;
        }
        false
    }

    // --- accessors ---

    pub fn set_input(&mut self, value: &str) {
        self.input = value.to_string();
        self.caret_pos = self.input.len();
        fire!(self, update_notifier);
    }

    pub fn input(&self) -> &str {
        &self.input
    }

    pub fn set_caret_pos(&mut self, caret_pos: usize) {
        self.caret_pos = caret_pos.min(self.input.len());
        fire!(self, update_notifier);
    }

    pub fn caret_pos(&self) -> usize {
        self.caret_pos
    }

    pub fn set_composition(&mut self, comp: Composition) {
        self.composition = comp;
    }

    pub fn composition(&self) -> &Composition {
        &self.composition
    }

    pub fn composition_mut(&mut self) -> &mut Composition {
        &mut self.composition
    }

    pub fn commit_history(&self) -> &CommitHistory {
        &self.commit_history
    }

    pub fn commit_history_mut(&mut self) -> &mut CommitHistory {
        &mut self.commit_history
    }

    pub fn set_option(&mut self, name: &str, value: bool) {
        self.options.insert(name.to_string(), value);
        fire!(self, option_update_notifier, name);
    }

    pub fn get_option(&self, name: &str) -> bool {
        self.options.get(name).copied().unwrap_or(false)
    }

    pub fn set_property(&mut self, name: &str, value: &str) {
        self.properties.insert(name.to_string(), value.to_string());
        fire!(self, property_update_notifier, name);
    }

    pub fn get_property(&self, name: &str) -> String {
        self.properties.get(name).cloned().unwrap_or_default()
    }

    pub fn options(&self) -> &BTreeMap<String, bool> {
        &self.options
    }

    pub fn properties(&self) -> &BTreeMap<String, String> {
        &self.properties
    }

    /// Options and properties starting with '_' are local to the schema;
    /// others are session scoped.
    pub fn clear_transient_options(&mut self) {
        let opt_keys: Vec<String> = self
            .options
            .keys()
            .filter(|k| k.starts_with('_'))
            .cloned()
            .collect();
        for k in opt_keys {
            self.options.remove(&k);
        }
        let prop_keys: Vec<String> = self
            .properties
            .keys()
            .filter(|k| k.starts_with('_'))
            .cloned()
            .collect();
        for k in prop_keys {
            self.properties.remove(&k);
        }
    }

    fn get_soft_cursor(&self) -> String {
        if self.get_option("soft_cursor") {
            CARET_SYMBOL.to_string()
        } else {
            String::new()
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn push_and_pop_input() {
        let mut ctx = Context::new();
        assert!(ctx.push_input('a'));
        assert!(ctx.push_input('b'));
        assert_eq!(ctx.input(), "ab");
        assert!(ctx.pop_input(1));
        assert_eq!(ctx.input(), "a");
        assert_eq!(ctx.caret_pos(), 1);
    }

    #[test]
    fn notifier_fires_on_update() {
        use std::cell::Cell;
        use std::rc::Rc;
        let mut ctx = Context::new();
        let fired = Rc::new(Cell::new(0));
        let fired2 = Rc::clone(&fired);
        ctx.on_update(Box::new(move |_| fired2.set(fired2.get() + 1)));
        ctx.push_input('a');
        assert_eq!(fired.get(), 1);
    }

    #[test]
    fn commit_clears_composition() {
        let mut ctx = Context::new();
        ctx.push_input('a');
        assert!(ctx.is_composing());
        // no composition text without candidates, but commit still clears
        assert!(ctx.commit());
        assert!(!ctx.is_composing());
    }

    #[test]
    fn transient_options_cleared() {
        let mut ctx = Context::new();
        ctx.set_option("_auto_commit", true);
        ctx.set_option("ascii_mode", true);
        ctx.set_property("_local", "x");
        ctx.set_property("global", "y");
        ctx.clear_transient_options();
        assert!(!ctx.get_option("_auto_commit"));
        assert!(ctx.get_option("ascii_mode"));
        assert_eq!(ctx.get_property("_local"), "");
        assert_eq!(ctx.get_property("global"), "y");
    }
}
