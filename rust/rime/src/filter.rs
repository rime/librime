// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/filter.h — pipeline component trait.

use crate::candidate::CandidateList;
use crate::segmentation::Segment;
use crate::translation::TranslationRc;

/// A filter transforms the candidate stream produced by translators.
pub trait Filter {
    /// Wrap or transform `translation` and return the (possibly new) stream.
    fn apply(&mut self, translation: TranslationRc, candidates: &mut CandidateList)
        -> TranslationRc;

    /// Whether this filter applies to the given segment.
    fn applies_to_segment(&self, _segment: &Segment) -> bool {
        true
    }
}
