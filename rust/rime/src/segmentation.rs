// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/segmentation.h and src/rime/segmentation.cc to Rust.

use std::cell::RefCell;
use std::collections::BTreeSet;
use std::ops::{Deref, DerefMut};
use std::rc::Rc;

use crate::candidate::CandidateArc;
use crate::menu::Menu;

/// Status of a segment.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum SegmentStatus {
    Void = 0,
    Guess = 1,
    Selected = 2,
    Confirmed = 3,
}

const PARTIAL_SELECTION_TAG: &str = "partial";

#[derive(Clone)]
pub struct Segment {
    pub status: SegmentStatus,
    pub start: usize,
    pub end: usize,
    pub length: usize,
    pub tags: BTreeSet<String>,
    pub menu: Option<Rc<RefCell<Menu>>>,
    pub selected_index: usize,
    pub prompt: String,
}

impl Segment {
    pub fn new() -> Self {
        Self {
            status: SegmentStatus::Void,
            start: 0,
            end: 0,
            length: 0,
            tags: BTreeSet::new(),
            menu: None,
            selected_index: 0,
            prompt: String::new(),
        }
    }

    pub fn with_range(start: usize, end: usize) -> Self {
        Self {
            start,
            end,
            length: end.saturating_sub(start),
            ..Self::new()
        }
    }

    pub fn clear(&mut self) {
        self.status = SegmentStatus::Void;
        self.tags.clear();
        self.menu = None;
        self.selected_index = 0;
        self.prompt.clear();
    }

    pub fn close(&mut self) {
        let cand_end = self.get_selected_candidate().map(|c| c.end());
        if let Some(cand_end) = cand_end {
            if cand_end < self.end {
                self.end = cand_end;
                self.tags.insert(PARTIAL_SELECTION_TAG.to_string());
            }
        }
    }

    pub fn reopen(&mut self, caret_pos: usize) -> bool {
        if self.status < SegmentStatus::Selected {
            return false;
        }
        let original_end_pos = self.start + self.length;
        if original_end_pos == caret_pos {
            if self.end < original_end_pos {
                self.end = original_end_pos;
                self.tags.remove(PARTIAL_SELECTION_TAG);
            }
            self.status = SegmentStatus::Guess;
        } else {
            self.status = SegmentStatus::Void;
        }
        true
    }

    pub fn has_tag(&self, tag: &str) -> bool {
        self.tags.contains(tag)
    }

    pub fn has_any_tag_in(&self, tags: &[String]) -> bool {
        tags.iter().any(|tag| self.has_tag(tag))
    }

    pub fn get_candidate_at(&self, index: usize) -> Option<CandidateArc> {
        self.menu.as_ref().and_then(|m| m.borrow_mut().get_candidate_at(index))
    }

    pub fn get_selected_candidate(&self) -> Option<CandidateArc> {
        self.get_candidate_at(self.selected_index)
    }
}

impl Default for Segment {
    fn default() -> Self {
        Self::new()
    }
}

/// A sequence of segments covering the input.
#[derive(Clone, Default)]
pub struct Segmentation {
    segments: Vec<Segment>,
    input: String,
}

impl Segmentation {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn input(&self) -> &str {
        &self.input
    }

    pub fn reset(&mut self, new_input: &str) {
        // mark redo segmentation, while keeping user confirmed segments
        let old = self.input.as_bytes();
        let new = new_input.as_bytes();
        let mut diff_pos = 0usize;
        while diff_pos < old.len() && diff_pos < new.len() && old[diff_pos] == new[diff_pos] {
            diff_pos += 1;
        }

        // dispose segments that have changed
        let mut disposed = 0;
        while !self.segments.is_empty()
            && self.segments.last().map(|s| s.end).unwrap_or(0) > diff_pos
        {
            self.segments.pop();
            disposed += 1;
        }
        if disposed > 0 {
            self.forward();
        }

        self.input = new_input.to_string();
    }

    pub fn reset_num(&mut self, num_segments: usize) {
        if num_segments >= self.segments.len() {
            return;
        }
        self.segments.truncate(num_segments);
    }

    pub fn add_segment(&mut self, segment: Segment) -> bool {
        let start = self.get_current_start_position();
        if segment.start != start {
            // rule one: only examine segments left-aligned to the same position
            return false;
        }
        if self.segments.is_empty() {
            self.segments.push(segment);
            return true;
        }
        let last = self.segments.last_mut().unwrap();
        if last.end > segment.end {
            // rule two: always prefer the longer segment
        } else if last.end < segment.end {
            // ...and overwrite the shorter one
            *last = segment;
        } else {
            // rule three: with segments equal in length, merge their tags
            let merged: BTreeSet<String> =
                last.tags.union(&segment.tags).cloned().collect();
            last.tags = merged;
        }
        true
    }

    /// Finalize a round.
    pub fn forward(&mut self) -> bool {
        if self.segments.is_empty()
            || self.segments.last().map(|s| s.start == s.end).unwrap_or(false)
        {
            return false;
        }
        let end = self.segments.last().unwrap().end;
        self.segments.push(Segment::with_range(end, end));
        true
    }

    /// Remove an empty trailing segment.
    pub fn trim(&mut self) -> bool {
        if !self.segments.is_empty()
            && self.segments.last().map(|s| s.start == s.end).unwrap_or(false)
        {
            self.segments.pop();
            return true;
        }
        false
    }

    pub fn has_finished_segmentation(&self) -> bool {
        self.segments.last().map(|s| s.end).unwrap_or(0) >= self.input.len()
    }

    pub fn get_current_start_position(&self) -> usize {
        self.segments.last().map(|s| s.start).unwrap_or(0)
    }

    pub fn get_current_end_position(&self) -> usize {
        self.segments.last().map(|s| s.end).unwrap_or(0)
    }

    pub fn get_current_segment_length(&self) -> usize {
        self.segments.last().map(|s| s.end - s.start).unwrap_or(0)
    }

    pub fn get_confirmed_position(&self) -> usize {
        let mut k = 0;
        for seg in &self.segments {
            if seg.status >= SegmentStatus::Selected {
                k = seg.end;
            }
        }
        k
    }
}

impl Deref for Segmentation {
    type Target = Vec<Segment>;
    fn deref(&self) -> &Vec<Segment> {
        &self.segments
    }
}

impl DerefMut for Segmentation {
    fn deref_mut(&mut self) -> &mut Vec<Segment> {
        &mut self.segments
    }
}

impl std::fmt::Display for Segmentation {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "[{}", self.input)?;
        for segment in &self.segments {
            write!(f, "|{},{}", segment.start, segment.end)?;
            if !segment.tags.is_empty() {
                write!(f, "{{{}}}", segment.tags.iter().cloned().collect::<Vec<_>>().join(","))?;
            }
        }
        write!(f, "]")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn add_segment_prefers_longer() {
        let mut s = Segmentation::new();
        s.reset("hello");
        assert!(s.add_segment(Segment::with_range(0, 2)));
        assert!(s.add_segment(Segment::with_range(0, 3)));
        assert_eq!(s.last().unwrap().end, 3);
        assert_eq!(s.len(), 1);
    }

    #[test]
    fn reset_keeps_confirmed_prefix() {
        let mut s = Segmentation::new();
        s.reset("hello");
        s.add_segment(Segment::with_range(0, 2));
        s.forward();
        s.add_segment(Segment::with_range(2, 4));
        // changing input after position 2 disposes the later segment
        s.reset("hexxx");
        assert_eq!(s.len(), 2); // segment [0,2) + trailing [2,2)
    }

    #[test]
    fn forward_and_trim() {
        let mut s = Segmentation::new();
        s.reset("abc");
        s.add_segment(Segment::with_range(0, 1));
        assert!(s.forward());
        assert_eq!(s.len(), 2);
        assert!(s.trim());
        assert_eq!(s.len(), 1);
    }
}
