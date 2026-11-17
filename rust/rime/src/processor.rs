// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/processor.h — pipeline component trait.

use crate::key_event::KeyEvent;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProcessResult {
    /// Do the OS default processing.
    Rejected,
    /// Consume it.
    Accepted,
    /// Leave it to other processors.
    Noop,
}

/// A processor handles key events before translation.
pub trait Processor {
    fn process_key_event(&mut self, key_event: &KeyEvent) -> ProcessResult;
}
