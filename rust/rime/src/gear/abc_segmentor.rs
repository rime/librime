// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/gear/abc_segmentor.cc to Rust.

use std::collections::BTreeSet;

use crate::segmentation::{Segment, Segmentation};
use crate::segmentor::Segmentor;
use crate::ticket::Ticket;

const RIME_ALPHABET: &str = "zyxwvutsrqponmlkjihgfedcba";

/// Segments the input into runs of alphabet letters/delimiters.
pub struct AbcSegmentor {
    alphabet: String,
    delimiter: String,
    initials: String,
    finals: String,
    extra_tags: BTreeSet<String>,
}

impl AbcSegmentor {
    pub fn new(ticket: &Ticket) -> Self {
        let config = &ticket.schema_config;
        let mut alphabet = RIME_ALPHABET.to_string();
        let mut delimiter = String::new();
        let mut initials = String::new();
        let mut finals = String::new();
        let mut extra_tags = BTreeSet::new();
        if let Some(v) = config.get_string("speller/alphabet") {
            alphabet = v.to_string();
        }
        if let Some(v) = config.get_string("speller/delimiter") {
            delimiter = v.to_string();
        }
        if let Some(v) = config.get_string("speller/initials") {
            initials = v.to_string();
        }
        if let Some(v) = config.get_string("speller/finals") {
            finals = v.to_string();
        }
        if let Some(list) = config.get_list("abc_segmentor/extra_tags") {
            for item in list {
                if let Some(v) = item.as_str() {
                    extra_tags.insert(v.to_string());
                }
            }
        }
        if initials.is_empty() {
            initials = alphabet.clone();
        }
        Self {
            alphabet,
            delimiter,
            initials,
            finals,
            extra_tags,
        }
    }
}

impl Segmentor for AbcSegmentor {
    fn proceed(&mut self, segmentation: &mut Segmentation) -> bool {
        let input = segmentation.input().to_string();
        let j = segmentation.get_current_start_position();
        let bytes = input.as_bytes();
        let mut k = j;
        let mut expecting_an_initial = true;
        while k < bytes.len() {
            let c = bytes[k] as char;
            let is_letter = self.alphabet.contains(c);
            let is_delimiter = k != 0 && self.delimiter.contains(c);
            if !is_letter && !is_delimiter {
                break;
            }
            let is_initial = self.initials.contains(c);
            let is_final = self.finals.contains(c);
            if expecting_an_initial && !is_initial && !is_delimiter {
                break; // not a valid spelling
            }
            expecting_an_initial = is_final || is_delimiter;
            k += 1;
        }
        if j < k {
            let mut segment = Segment::with_range(j, k);
            segment.tags.insert("abc".to_string());
            for tag in &self.extra_tags {
                segment.tags.insert(tag.clone());
            }
            segmentation.add_segment(segment);
        }
        true
    }
}
