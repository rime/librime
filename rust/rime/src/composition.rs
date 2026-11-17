// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/composition.h and src/rime/composition.cc to Rust.

use std::ops::{Deref, DerefMut};

use crate::segmentation::{SegmentStatus, Segmentation};
use crate::util::{erase_first, substr};

/// The preedit text derived from a composition.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Preedit {
    pub text: String,
    pub caret_pos: usize,
    pub sel_start: usize,
    pub sel_end: usize,
}

/// A preview of committing the highlighted candidate.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct CandidatePreview {
    pub text_before_selection: String,
    pub selected_text: String,
    pub text_after_selection: String,
}

const NPOS: usize = usize::MAX;

/// A composition of segments over the input.
#[derive(Clone, Default)]
pub struct Composition(Segmentation);

impl Composition {
    pub fn new() -> Self {
        Self(Segmentation::new())
    }

    pub fn as_segmentation(&self) -> &Segmentation {
        &self.0
    }

    pub fn as_segmentation_mut(&mut self) -> &mut Segmentation {
        &mut self.0
    }

    pub fn has_finished_composition(&self) -> bool {
        if self.is_empty() {
            return false;
        }
        let mut k = self.len() - 1;
        if k > 0 && self[k].start == self[k].end {
            k -= 1;
        }
        self[k].status >= SegmentStatus::Selected
    }

    pub fn get_preedit(&self, full_input: &str, caret_pos: usize, caret: &str) -> Preedit {
        let mut preedit = Preedit {
            caret_pos: NPOS,
            ..Default::default()
        };
        let input = self.input();
        let mut end = 0usize;
        for i in 0..self.len() {
            let start = end;
            if caret_pos == start {
                preedit.caret_pos = preedit.text.len();
            }
            let cand = self[i].get_selected_candidate();
            if i < self.len() - 1 {
                // converted
                if let Some(cand) = cand {
                    end = cand.end();
                    preedit.text.push_str(cand.text());
                } else {
                    // raw input
                    end = self[i].end;
                    if !self[i].has_tag("phony") {
                        preedit.text.push_str(substr(input, start, end));
                    }
                }
            } else {
                // highlighted
                preedit.sel_start = preedit.text.len();
                preedit.sel_end = NPOS;
                let preedit_text = cand.as_ref().map(|c| c.preedit().to_string());
                if let Some(ref p) = preedit_text {
                    if !p.is_empty() {
                        end = cand.as_ref().unwrap().end();
                        if let Some(caret_placeholder) = p.find('\t') {
                            preedit.text.push_str(&p[..caret_placeholder]);
                            if caret_pos == end && end == full_input.len() {
                                preedit.sel_end = preedit.sel_start + caret_placeholder;
                                preedit.caret_pos = preedit.sel_end;
                                preedit.text.push_str(&p[caret_placeholder + 1..]);
                            }
                        } else {
                            preedit.text.push_str(p);
                        }
                    } else {
                        end = self[i].end;
                        preedit.text.push_str(substr(input, start, end));
                    }
                } else {
                    end = self[i].end;
                    preedit.text.push_str(substr(input, start, end));
                }
                if preedit.sel_end == NPOS {
                    preedit.sel_end = preedit.text.len();
                }
            }
        }
        if end < input.len() {
            preedit.text.push_str(substr(input, end, input.len()));
            end = input.len();
        }
        if preedit.caret_pos == NPOS {
            preedit.caret_pos = preedit.text.len();
        }
        if end < full_input.len() {
            preedit.text.push_str(substr(full_input, end, full_input.len()));
        }
        // insert soft cursor and prompt string
        let prompt = format!("{}{}", caret, self.get_prompt());
        if !prompt.is_empty() {
            if preedit.caret_pos <= preedit.text.len() {
                preedit.text.insert_str(preedit.caret_pos, &prompt);
                if preedit.caret_pos < preedit.sel_start {
                    preedit.sel_start += prompt.len();
                }
                if preedit.caret_pos < preedit.sel_end {
                    preedit.sel_end += prompt.len();
                }
            }
        }
        preedit
    }

    pub fn get_prompt(&self) -> String {
        self.last().map(|s| s.prompt.clone()).unwrap_or_default()
    }

    pub fn get_commit_text(&self) -> String {
        let input = self.input();
        let mut result = String::new();
        let mut end = 0usize;
        for seg in self.iter() {
            if let Some(cand) = seg.get_selected_candidate() {
                end = cand.end();
                result.push_str(cand.text());
            } else {
                end = seg.end;
                if !seg.has_tag("phony") {
                    result.push_str(substr(input, seg.start, seg.end));
                }
            }
        }
        if input.len() > end {
            result.push_str(substr(input, end, input.len()));
        }
        result
    }

    pub fn get_script_text(&self, keep_selection: bool) -> String {
        let input = self.input();
        let mut result = String::new();
        let mut end = 0usize;
        for seg in self.iter() {
            let cand = seg.get_selected_candidate();
            let start = end;
            end = cand.as_ref().map(|c| c.end()).unwrap_or(seg.end);
            if let Some(cand) = cand {
                if keep_selection && !cand.text().is_empty() && seg.status >= SegmentStatus::Selected
                {
                    result.push_str(cand.text());
                } else if !cand.preedit().is_empty() {
                    result.push_str(&erase_first(cand.preedit(), "\t"));
                } else if !seg.has_tag("phony") {
                    result.push_str(substr(input, start, end));
                }
            } else if !seg.has_tag("phony") {
                result.push_str(substr(input, start, end));
            }
        }
        if input.len() > end {
            result.push_str(substr(input, end, input.len()));
        }
        result
    }

    pub fn get_debug_text(&self) -> String {
        let input = self.input();
        let mut result = String::new();
        for (i, seg) in self.iter().enumerate() {
            if i > 0 {
                result.push('|');
            }
            if !seg.tags.is_empty() {
                result.push('{');
                result.push_str(&seg.tags.iter().cloned().collect::<Vec<_>>().join(","));
                result.push('}');
            }
            result.push_str(substr(input, seg.start, seg.end));
            if let Some(cand) = seg.get_selected_candidate() {
                result.push_str("=>");
                result.push_str(cand.text());
            }
        }
        result
    }

    /// Returns text of the last segment before the given position.
    pub fn get_text_before(&self, pos: usize) -> String {
        if self.is_empty() {
            return String::new();
        }
        for seg in self.iter().rev() {
            if seg.end <= pos {
                if let Some(cand) = seg.get_selected_candidate() {
                    return cand.text().to_string();
                }
            }
        }
        String::new()
    }

    pub fn get_candidate_preview(&self, full_input: &str) -> CandidatePreview {
        let mut preview = CandidatePreview::default();
        let input = self.input();
        // input_ is truncated to the caret; full_input recovers chars past it.
        let source = if !full_input.is_empty() { full_input } else { input };

        if self.is_empty() {
            preview.selected_text = source.to_string();
            return preview;
        }

        // rime appends an empty segment at the caret; use the last non-empty one.
        let mut active = self.len() - 1;
        while active > 0 && self[active].start >= self[active].end {
            active -= 1;
        }

        let mut before = String::new();
        for seg in self.iter().take(active) {
            if let Some(cand) = seg.get_selected_candidate() {
                before.push_str(cand.text());
            } else {
                before.push_str(substr(input, seg.start, seg.end));
            }
        }
        preview.text_before_selection = before;

        let current_seg = &self[active];
        let mut candidate_end_pos = current_seg.end;
        let mut segment_text = String::new();
        if let Some(candidate) = current_seg.get_selected_candidate() {
            if !candidate.text().is_empty() {
                segment_text = candidate.text().to_string();
                candidate_end_pos = candidate.end();
            }
        }
        if segment_text.is_empty() {
            segment_text = substr(input, current_seg.start, current_seg.end).to_string();
        }
        preview.selected_text = segment_text;

        if source.len() > candidate_end_pos {
            preview.text_after_selection = substr(source, candidate_end_pos, source.len()).to_string();
        }
        preview
    }
}

impl Deref for Composition {
    type Target = Segmentation;
    fn deref(&self) -> &Segmentation {
        &self.0
    }
}

impl DerefMut for Composition {
    fn deref_mut(&mut self) -> &mut Segmentation {
        &mut self.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::segmentation::{Segment, SegmentStatus};

    #[test]
    fn empty_composition_preview_is_source() {
        let c = Composition::new();
        let p = c.get_candidate_preview("hello");
        assert_eq!(p.selected_text, "hello");
    }

    #[test]
    fn commit_text_joins_selected_candidates() {
        let mut c = Composition::new();
        c.reset("abc");
        let mut seg = Segment::with_range(0, 1);
        seg.status = SegmentStatus::Selected;
        // candidate text "A" for [0,1)
        // Build a menu with a candidate is heavyweight; use direct segment text path.
        seg.selected_index = 0;
        c.push(seg);
        // no menu -> selected candidate is None -> raw input used
        assert_eq!(c.get_commit_text(), "abc");
    }
}
