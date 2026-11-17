// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/translator.h — pipeline component trait.

use crate::segmentation::Segment;
use crate::translation::TranslationRc;

/// A translator produces candidates for a segment of input.
pub trait Translator {
    fn query(&mut self, input: &str, segment: &Segment) -> Option<TranslationRc>;
}
