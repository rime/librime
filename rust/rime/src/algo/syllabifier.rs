// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/algo/syllabifier.h — a simplified syllable graph builder.
// This omits correction, canonicalization and the spelling-type preference
// cleanup of the original; it finds all segmentations of the input into
// syllables using a spelling prism.

use std::collections::{BTreeMap, BTreeSet, VecDeque};

use super::spelling::SpellingProperties;
use crate::dict::prism::Prism;
use crate::dict::vocabulary::SyllableId;

pub type SpellingMap = BTreeMap<SyllableId, SpellingProperties>;
pub type EndVertexMap = BTreeMap<usize, SpellingMap>;
pub type EdgeMap = BTreeMap<usize, EndVertexMap>;

/// A DAG of possible syllable segmentations of the input.
pub struct SyllableGraph {
    pub input_length: usize,
    pub interpreted_length: usize,
    pub edges: EdgeMap,
}

pub struct Syllabifier {
    delimiters: String,
}

impl Syllabifier {
    pub fn new(delimiters: &str) -> Self {
        Self {
            delimiters: delimiters.to_string(),
        }
    }

    pub fn build_syllable_graph(&self, input: &str, prism: &Prism) -> SyllableGraph {
        let mut graph = SyllableGraph {
            input_length: input.len(),
            interpreted_length: 0,
            edges: EdgeMap::new(),
        };
        if input.is_empty() {
            return graph;
        }
        let bytes = input.as_bytes();
        let mut visited = BTreeSet::new();
        let mut queue = VecDeque::new();
        queue.push_back(0usize);

        while let Some(current_pos) = queue.pop_front() {
            if !visited.insert(current_pos) {
                continue;
            }
            // consume leading delimiters
            let mut begin_pos = current_pos;
            while begin_pos < bytes.len() && self.delimiters.contains(bytes[begin_pos] as char) {
                begin_pos += 1;
            }
            let rest = &input[begin_pos..];
            let matches = prism.common_prefix_search(rest);
            if matches.is_empty() {
                continue;
            }
            let end_vertices = graph.edges.entry(current_pos).or_default();
            for (syllable_id, len) in matches {
                if len == 0 {
                    continue;
                }
                let mut end_pos = begin_pos + len;
                while end_pos < bytes.len() && self.delimiters.contains(bytes[end_pos] as char) {
                    end_pos += 1;
                }
                graph.interpreted_length = graph.interpreted_length.max(end_pos);
                let spellings = end_vertices.entry(end_pos).or_default();
                spellings
                    .entry(syllable_id)
                    .or_insert_with(|| SpellingProperties {
                        end_pos,
                        ..Default::default()
                    });
                queue.push_back(end_pos);
            }
        }
        graph
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::dict::prism::Prism;

    #[test]
    fn builds_graph_for_undelimited_input() {
        let prism = Prism::from_syllabary(&["ni".into(), "hao".into(), "zhong".into(), "guo".into()]);
        let s = Syllabifier::new("");
        let g = s.build_syllable_graph("nihao", &prism);
        assert_eq!(g.interpreted_length, 5);
        assert!(g.edges.contains_key(&0));
        let from0 = &g.edges[&0];
        assert!(from0.contains_key(&2)); // "ni"
        assert!(g.edges[&2].contains_key(&5)); // "hao"
    }
}
