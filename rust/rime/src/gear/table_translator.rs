// Copyright RIME Developers
// Distributed under the BSD License
//
// A minimal table translator. It looks up the (delimiter-split) segment input
// in a dictionary and yields one candidate per matching entry. This is a
// reduced port of src/rime/gear/table_translator.cc, omitting user dictionaries,
// sentence composition, encoders and charset filtering.

use std::rc::Rc;
use std::sync::Arc;

use crate::candidate::{Candidate, CandidateArc, CandidateBase};
use crate::dict::dictionary::{find_dictionary, Dictionary};
use crate::dict::vocabulary::DictEntry;
use crate::segmentation::Segment;
use crate::ticket::Ticket;
use crate::translation::{new_translation, FifoTranslation, TranslationRc};
use crate::translator::Translator;

/// A candidate backed by a dictionary entry.
#[derive(Clone)]
pub struct Phrase {
    base: CandidateBase,
    entry: DictEntry,
}

impl Phrase {
    pub fn new(type_: &str, start: usize, end: usize, entry: DictEntry) -> Self {
        Self {
            base: CandidateBase {
                type_: type_.to_string(),
                start,
                end,
                quality: 0.0,
            },
            entry,
        }
    }
}

impl Candidate for Phrase {
    fn base(&self) -> &CandidateBase {
        &self.base
    }
    fn base_mut(&mut self) -> &mut CandidateBase {
        &mut self.base
    }
    fn text(&self) -> &str {
        &self.entry.text
    }
    fn comment(&self) -> &str {
        &self.entry.comment
    }
    fn preedit(&self) -> &str {
        &self.entry.preedit
    }
    fn clone_candidate(&self) -> CandidateArc {
        Arc::new(self.clone())
    }
}

/// A translator that maps code sequences to dictionary entries.
pub struct TableTranslator {
    dictionary: Option<Rc<Dictionary>>,
    delimiter: String,
    max_homographs: usize,
    initial_quality: f64,
}

impl TableTranslator {
    pub fn new(ticket: &Ticket) -> Self {
        let config = &ticket.schema_config;
        let dict_name = config
            .get_string("translator/dictionary")
            .map(|s| s.to_string())
            .unwrap_or_default();
        let dictionary = if dict_name.is_empty() {
            None
        } else {
            find_dictionary(&dict_name)
        };
        let delimiter = config
            .get_string("translator/delimiter")
            .unwrap_or(" ")
            .to_string();
        let max_homographs = config.get_int("translator/max_homographs").unwrap_or(1).max(1) as usize;
        let initial_quality = config.get_double("translator/initial_quality").unwrap_or(0.0);
        Self {
            dictionary,
            delimiter,
            max_homographs,
            initial_quality,
        }
    }
}

impl Translator for TableTranslator {
    fn query(&mut self, input: &str, segment: &Segment) -> Option<TranslationRc> {
        let dict = self.dictionary.as_ref()?;
        if input.is_empty() {
            return None;
        }
        let code = input.trim_end_matches(|c| self.delimiter.contains(c));
        let syllables: Vec<&str> = if self.delimiter.is_empty() {
            vec![code]
        } else {
            code.split(|c| self.delimiter.contains(c))
                .filter(|s| !s.is_empty())
                .collect()
        };
        let entries = dict.lookup_words(&syllables, false);
        if entries.is_empty() {
            return None;
        }
        let mut fifo = FifoTranslation::new();
        for entry in entries.iter().take(self.max_homographs) {
            let mut phrase = Phrase::new("table", segment.start, segment.end, entry.clone());
            phrase.set_quality(entry.weight.exp() + self.initial_quality);
            fifo.append(Arc::new(phrase));
        }
        Some(new_translation(fifo))
    }
}
