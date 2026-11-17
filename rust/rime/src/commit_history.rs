// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/commit_history.h and src/rime/commit_history.cc to Rust.

use std::ops::{Deref, DerefMut};

use crate::composition::Composition;
use crate::key_event::KeyEvent;
use crate::key_table::{XK_BACK_SPACE, XK_RETURN};
use crate::segmentation::SegmentStatus;
use crate::util::substr;

/// A single commit record.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommitRecord {
    pub type_: String,
    pub text: String,
}

impl CommitRecord {
    pub fn new(type_: &str, text: &str) -> Self {
        Self {
            type_: type_.to_string(),
            text: text.to_string(),
        }
    }

    pub fn from_keycode(keycode: i32) -> Self {
        Self {
            type_: "thru".to_string(),
            text: (keycode as u8 as char).to_string(),
        }
    }
}

const K_MAX_RECORDS: usize = 20;

/// A bounded history of commits.
#[derive(Debug, Clone, Default)]
pub struct CommitHistory(Vec<CommitRecord>);

impl CommitHistory {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn push(&mut self, record: CommitRecord) {
        self.0.push(record);
        if !self.0.is_empty() && self.0.len() > K_MAX_RECORDS {
            self.0.remove(0);
        }
    }

    pub fn push_key_event(&mut self, key_event: &KeyEvent) {
        if key_event.modifier() != 0 {
            return;
        }
        let keycode = key_event.keycode();
        if keycode == XK_BACK_SPACE || keycode == XK_RETURN {
            self.clear();
        } else if (0x20..=0x7e).contains(&keycode) {
            self.push(CommitRecord::from_keycode(keycode));
        }
    }

    pub fn push_composition(&mut self, composition: &Composition, input: &str) {
        let mut last: Option<usize> = None;
        let mut end = 0usize;
        for seg in composition.iter() {
            if let Some(cand) = seg.get_selected_candidate() {
                let ctype = cand.type_name().to_string();
                let ctext = cand.text().to_string();
                let mut joined = false;
                if let Some(li) = last {
                    if self.0.get(li).map(|r| r.type_ == ctype).unwrap_or(false) {
                        if let Some(r) = self.0.get_mut(li) {
                            r.text.push_str(&ctext);
                        }
                        joined = true;
                    }
                }
                if !joined {
                    self.push(CommitRecord::new(&ctype, &ctext));
                    last = Some(self.0.len() - 1);
                }
                if seg.status >= SegmentStatus::Confirmed {
                    last = None;
                }
                end = cand.end();
            } else {
                // no translation for the segment
                self.push(CommitRecord::new("raw", substr(input, seg.start, seg.end)));
                end = seg.end;
            }
        }
        if input.len() > end {
            self.push(CommitRecord::new("raw", substr(input, end, input.len())));
        }
    }

    pub fn repr(&self) -> String {
        let mut result = String::new();
        for record in &self.0 {
            result.push_str(&format!("[{}]{}", record.type_, record.text));
        }
        result
    }

    pub fn latest_text(&self) -> String {
        self.0.last().map(|r| r.text.clone()).unwrap_or_default()
    }

    pub fn clear(&mut self) {
        self.0.clear();
    }
}

impl Deref for CommitHistory {
    type Target = Vec<CommitRecord>;
    fn deref(&self) -> &Vec<CommitRecord> {
        &self.0
    }
}

impl DerefMut for CommitHistory {
    fn deref_mut(&mut self) -> &mut Vec<CommitRecord> {
        &mut self.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn caps_history_at_20() {
        let mut h = CommitHistory::new();
        for i in 0..25 {
            h.push(CommitRecord::from_keycode(0x61 + i));
        }
        assert_eq!(h.len(), 20);
        assert_eq!(h.latest_text(), "y");
    }

    #[test]
    fn push_key_event_records_printable() {
        let mut h = CommitHistory::new();
        h.push_key_event(&KeyEvent::with_keycode(b'a' as i32, 0));
        assert_eq!(h.latest_text(), "a");
        h.push_key_event(&KeyEvent::with_keycode(XK_BACK_SPACE, 0));
        assert!(h.is_empty());
    }
}
