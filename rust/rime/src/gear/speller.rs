// Copyright RIME Developers
// Distributed under the BSD License
//
// A minimal speller processor: it pushes printable alphabet/delimiter keys into
// the input, pops on BackSpace and selects on Space. A reduced port of
// src/rime/gear/speller.cc (omitting auto-select/auto-clear heuristics).

use std::cell::RefCell;
use std::rc::Rc;

use crate::context::Context;
use crate::key_event::KeyEvent;
use crate::key_table::{XK_BACK_SPACE, XK_SPACE};
use crate::processor::{ProcessResult, Processor};
use crate::ticket::Ticket;

const RIME_ALPHABET: &str = "zyxwvutsrqponmlkjihgfedcba";

pub struct Speller {
    context: Rc<RefCell<Context>>,
    alphabet: String,
    delimiters: String,
    use_space: bool,
}

impl Speller {
    pub fn new(ticket: &Ticket) -> Self {
        let config = &ticket.schema_config;
        let mut alphabet = RIME_ALPHABET.to_string();
        let mut delimiters = String::new();
        let mut use_space = false;
        if let Some(v) = config.get_string("speller/alphabet") {
            alphabet = v.to_string();
        }
        if let Some(v) = config.get_string("speller/delimiter") {
            delimiters = v.to_string();
        }
        if let Some(v) = config.get_bool("speller/use_space") {
            use_space = v;
        }
        Self {
            context: ticket.context.clone(),
            alphabet,
            delimiters,
            use_space,
        }
    }
}

impl Processor for Speller {
    fn process_key_event(&mut self, key_event: &KeyEvent) -> ProcessResult {
        if key_event.release() || key_event.ctrl() || key_event.alt() || key_event.super_key() {
            return ProcessResult::Noop;
        }
        let ch = key_event.keycode();

        if ch == XK_BACK_SPACE {
            self.context.borrow_mut().pop_input(1);
            return ProcessResult::Accepted;
        }

        if ch < 0x20 || ch >= 0x7f {
            return ProcessResult::Noop;
        }
        if ch == XK_SPACE {
            if !self.use_space || key_event.shift() {
                return ProcessResult::Noop;
            }
            let mut ctx = self.context.borrow_mut();
            if ctx.is_composing() {
                if !ctx.select(0) {
                    ctx.push_input(' ');
                }
                return ProcessResult::Accepted;
            }
            return ProcessResult::Noop;
        }

        let c = ch as u8 as char;
        if !self.alphabet.contains(c) && !self.delimiters.contains(c) {
            return ProcessResult::Noop;
        }
        self.context.borrow_mut().push_input(c);
        ProcessResult::Accepted
    }
}
