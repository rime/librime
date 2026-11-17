// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/component.h and src/rime/registry.h to Rust.
//
// Component factories are registered in per-kind registries keyed by name.
// This mirrors the C++ `Class<T>::Require(name)` / `Registry::Register` scheme
// without needing type-erased downcasting: each component kind has a uniform
// factory signature.

use std::collections::HashMap;
use std::sync::{Mutex, OnceLock};

use crate::filter::Filter;
use crate::formatter::Formatter;
use crate::processor::Processor;
use crate::segmentor::Segmentor;
use crate::ticket::Ticket;
use crate::translator::Translator;

pub type ProcessorFactory = fn(&Ticket) -> Option<Box<dyn Processor>>;
pub type SegmentorFactory = fn(&Ticket) -> Option<Box<dyn Segmentor>>;
pub type TranslatorFactory = fn(&Ticket) -> Option<Box<dyn Translator>>;
pub type FilterFactory = fn(&Ticket) -> Option<Box<dyn Filter>>;
pub type FormatterFactory = fn(&Ticket) -> Option<Box<dyn Formatter>>;

macro_rules! define_registry {
    ($reg:ident, $factory:ty, $register:ident, $require:ident) => {
        fn $reg() -> &'static Mutex<HashMap<String, $factory>> {
            static REG: OnceLock<Mutex<HashMap<String, $factory>>> = OnceLock::new();
            REG.get_or_init(|| Mutex::new(HashMap::new()))
        }

        pub fn $register(name: &str, factory: $factory) {
            $reg().lock().unwrap().insert(name.to_string(), factory);
        }

        pub fn $require(name: &str) -> Option<$factory> {
            $reg().lock().unwrap().get(name).copied()
        }
    };
}

define_registry!(processor_registry, ProcessorFactory, register_processor, require_processor);
define_registry!(segmentor_registry, SegmentorFactory, register_segmentor, require_segmentor);
define_registry!(translator_registry, TranslatorFactory, register_translator, require_translator);
define_registry!(filter_registry, FilterFactory, register_filter, require_filter);
define_registry!(formatter_registry, FormatterFactory, register_formatter, require_formatter);
