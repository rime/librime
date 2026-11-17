// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/formatter.h — pipeline component trait.

/// A formatter post-processes the text before it is committed.
pub trait Formatter {
    fn format(&mut self, text: &mut String);
}
