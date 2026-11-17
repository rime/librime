// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/segmentor.h — pipeline component trait.

use crate::segmentation::Segmentation;

/// A segmentor recognizes a segment of the input.
pub trait Segmentor {
    fn proceed(&mut self, segmentation: &mut Segmentation) -> bool;
}
