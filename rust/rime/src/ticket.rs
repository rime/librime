// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/ticket.h and src/rime/ticket.cc to Rust.

use std::cell::RefCell;
use std::rc::Rc;

use crate::config::Config;
use crate::context::Context;

/// Everything a component needs at construction time.
#[derive(Clone)]
pub struct Ticket {
    pub context: Rc<RefCell<Context>>,
    pub schema_config: Rc<Config>,
    pub name_space: String,
    pub klass: String,
}

impl Ticket {
    /// `prescription` is in the form `klass` or `klass@alias`, where the alias,
    /// if given, overrides the default name space.
    pub fn new(
        context: Rc<RefCell<Context>>,
        schema_config: Rc<Config>,
        name_space: &str,
        prescription: &str,
    ) -> Self {
        let mut klass = prescription.to_string();
        let mut ns = name_space.to_string();
        if let Some(separator) = klass.find('@') {
            ns = klass[separator + 1..].to_string();
            klass.truncate(separator);
        }
        Self {
            context,
            schema_config,
            name_space: ns,
            klass,
        }
    }
}
