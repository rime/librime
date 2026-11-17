// Copyright RIME Developers
// Distributed under the BSD License
//
// A minimal selector processor: it maps number keys / alternative select keys
// to candidate indices and handles Up/Down/Left/Right/PageUp/PageDown for
// candidate navigation. A reduced port of src/rime/gear/selector.cc.

use std::cell::RefCell;
use std::rc::Rc;

use crate::context::Context;
use crate::key_event::KeyEvent;
use crate::key_table::{XK_DOWN, XK_LEFT, XK_PAGE_DOWN, XK_PAGE_UP, XK_RIGHT, XK_UP};
use crate::processor::{ProcessResult, Processor};
use crate::ticket::Ticket;

pub struct Selector {
    context: Rc<RefCell<Context>>,
    page_size: usize,
    select_keys: String,
}

impl Selector {
    pub fn new(ticket: &Ticket) -> Self {
        let config = &ticket.schema_config;
        let page_size = config.get_int("menu/page_size").unwrap_or(5).max(1) as usize;
        let select_keys = config
            .get_string("menu/alternative_select_keys")
            .unwrap_or("")
            .to_string();
        Self {
            context: ticket.context.clone(),
            page_size,
            select_keys,
        }
    }

    fn select_candidate_at(&self, ctx: &mut Context, index: usize) -> bool {
        if ctx.composition().is_empty() || index >= self.page_size {
            return false;
        }
        let selected = ctx
            .composition()
            .last()
            .map(|s| s.selected_index)
            .unwrap_or(0);
        let page_start = (selected / self.page_size) * self.page_size;
        ctx.select(page_start + index)
    }

    fn previous_candidate(&self, ctx: &mut Context) -> bool {
        if ctx.composition().is_empty() {
            return false;
        }
        let index = ctx
            .composition()
            .last()
            .map(|s| s.selected_index)
            .unwrap_or(0);
        if index == 0 {
            return false;
        }
        ctx.highlight(index - 1)
    }

    fn next_candidate(&self, ctx: &mut Context) -> bool {
        if ctx.composition().is_empty() {
            return false;
        }
        let index = ctx
            .composition()
            .last()
            .map(|s| s.selected_index)
            .unwrap_or(0)
            + 1;
        ctx.highlight(index)
    }

    fn previous_page(&self, ctx: &mut Context) -> bool {
        if ctx.composition().is_empty() {
            return false;
        }
        let selected = ctx
            .composition()
            .last()
            .map(|s| s.selected_index)
            .unwrap_or(0);
        let index = if selected < self.page_size {
            0
        } else {
            selected - self.page_size
        };
        ctx.highlight(index)
    }

    fn next_page(&self, ctx: &mut Context) -> bool {
        if ctx.composition().is_empty() {
            return false;
        }
        let selected = ctx
            .composition()
            .last()
            .map(|s| s.selected_index)
            .unwrap_or(0);
        ctx.highlight(selected + self.page_size)
    }
}

impl Processor for Selector {
    fn process_key_event(&mut self, key_event: &KeyEvent) -> ProcessResult {
        if key_event.release() || key_event.alt() || key_event.super_key() {
            return ProcessResult::Noop;
        }
        let mut ctx = self.context.borrow_mut();
        if ctx.composition().is_empty()
            || ctx
                .composition()
                .last()
                .map(|s| s.menu.is_none() || s.has_tag("raw"))
                .unwrap_or(true)
        {
            return ProcessResult::Noop;
        }

        let ch = key_event.keycode();
        if !key_event.ctrl() && ch >= 0x20 && ch < 0x7f {
            let c = ch as u8 as char;
            if !self.select_keys.is_empty() {
                if let Some(pos) = self.select_keys.find(c) {
                    self.select_candidate_at(&mut *ctx, pos);
                    return ProcessResult::Accepted;
                }
            }
            if c.is_ascii_digit() {
                let index = ((c as i32 - '0' as i32) + 9) % 10;
                self.select_candidate_at(&mut *ctx, index as usize);
                return ProcessResult::Accepted;
            }
        }

        match ch {
            XK_UP | XK_LEFT => {
                self.previous_candidate(&mut *ctx);
                ProcessResult::Accepted
            }
            XK_DOWN | XK_RIGHT => {
                self.next_candidate(&mut *ctx);
                ProcessResult::Accepted
            }
            XK_PAGE_UP => {
                self.previous_page(&mut *ctx);
                ProcessResult::Accepted
            }
            XK_PAGE_DOWN => {
                self.next_page(&mut *ctx);
                ProcessResult::Accepted
            }
            _ => ProcessResult::Noop,
        }
    }
}
