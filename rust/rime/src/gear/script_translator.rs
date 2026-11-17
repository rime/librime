// Copyright RIME Developers
// Distributed under the BSD License
//
// A minimal script translator for phonetic input methods. It syllabifies the
// (undelimited) segment input using a spelling prism, enumerates the syllable
// sequences, and yields dictionary entries as candidates. This is a reduced
// port of src/rime/gear/script_translator.cc (no user dictionary, sentence
// composition, correction or word completion).

use std::rc::Rc;
use std::sync::Arc;

use super::table_translator::Phrase;
use crate::algo::syllabifier::{Syllabifier, SyllableGraph};
use crate::candidate::Candidate;
use crate::dict::dictionary::{find_dictionary, Dictionary};
use crate::dict::vocabulary::SyllableId;
use crate::segmentation::Segment;
use crate::ticket::Ticket;
use crate::translation::{new_translation, FifoTranslation, TranslationRc};
use crate::translator::Translator;

pub struct ScriptTranslator {
    dictionary: Option<Rc<Dictionary>>,
    syllabifier: Syllabifier,
    max_homophones: usize,
    initial_quality: f64,
}

impl ScriptTranslator {
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
        let delimiters = config
            .get_string("translator/delimiters")
            .unwrap_or("'")
            .to_string();
        let max_homophones = config.get_int("translator/max_homophones").unwrap_or(1).max(1) as usize;
        let initial_quality = config.get_double("translator/initial_quality").unwrap_or(0.0);
        Self {
            dictionary,
            syllabifier: Syllabifier::new(&delimiters),
            max_homophones,
            initial_quality,
        }
    }

    fn enumerate_codes(
        graph: &SyllableGraph,
        pos: usize,
        current: &mut Vec<SyllableId>,
        out: &mut Vec<Vec<SyllableId>>,
    ) {
        if pos == graph.input_length {
            out.push(current.clone());
            return;
        }
        if let Some(end_vertices) = graph.edges.get(&pos) {
            for (&end_pos, spellings) in end_vertices {
                for &syllable_id in spellings.keys() {
                    current.push(syllable_id);
                    Self::enumerate_codes(graph, end_pos, current, out);
                    current.pop();
                }
            }
        }
    }
}

impl Translator for ScriptTranslator {
    fn query(&mut self, input: &str, segment: &Segment) -> Option<TranslationRc> {
        let dict = self.dictionary.as_ref()?;
        if input.is_empty() {
            return None;
        }
        let graph = self.syllabifier.build_syllable_graph(input, dict.prism());
        if graph.interpreted_length < input.len() {
            return None; // input not fully syllabifiable
        }
        let mut codes = Vec::new();
        Self::enumerate_codes(&graph, 0, &mut Vec::new(), &mut codes);
        if codes.is_empty() {
            return None;
        }

        let mut seen = std::collections::BTreeSet::new();
        let mut fifo = FifoTranslation::new();
        for code in &codes {
            for entry in dict.lookup_code(code, false) {
                if !seen.insert(entry.text.clone()) {
                    continue;
                }
                let mut phrase = Phrase::new("phrase", segment.start, segment.end, entry.clone());
                phrase.set_quality(entry.weight.exp() + self.initial_quality);
                fifo.append(Arc::new(phrase));
                if fifo.size() >= self.max_homophones {
                    break;
                }
            }
        }
        if fifo.size() == 0 {
            return None;
        }
        Some(new_translation(fifo))
    }
}
