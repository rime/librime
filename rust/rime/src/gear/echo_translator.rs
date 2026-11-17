// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/gear/echo_translator.cc to Rust.

use std::sync::Arc;

use crate::candidate::{Candidate, SimpleCandidate};
use crate::segmentation::Segment;
use crate::ticket::Ticket;
use crate::translation::{new_translation, TranslationRc, UniqueTranslation};
use crate::translator::Translator;

/// A translator that echoes the segment input as a low-priority candidate.
pub struct EchoTranslator;

impl EchoTranslator {
    pub fn new(_ticket: &Ticket) -> Self {
        Self
    }
}

impl Translator for EchoTranslator {
    fn query(&mut self, input: &str, segment: &Segment) -> Option<TranslationRc> {
        if input.is_empty() {
            return None;
        }
        let mut candidate = SimpleCandidate::new("raw", segment.start, segment.end, input, "", "");
        candidate.set_quality(-100.0); // lowest priority
        Some(new_translation(UniqueTranslation::new(Some(Arc::new(
            candidate,
        )))))
    }
}
