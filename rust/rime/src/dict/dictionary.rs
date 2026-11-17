// Copyright RIME Developers
// Distributed under the BSD License
//
// A simplified Dictionary facade over an in-memory Table, plus a global
// dictionary store so translators can resolve a dictionary by name from config.

use std::cell::RefCell;
use std::collections::HashMap;
use std::rc::Rc;

use super::prism::Prism;
use super::table::Table;
use super::vocabulary::{DictEntry, SyllableId};

/// A dictionary: a name plus a lookup table and a spelling prism.
pub struct Dictionary {
    name: String,
    table: Rc<Table>,
    prism: Prism,
}

impl Dictionary {
    pub fn new(name: &str, table: Rc<Table>) -> Self {
        let prism = Prism::from_syllabary(table.syllabary());
        Self {
            name: name.to_string(),
            table,
            prism,
        }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn loaded(&self) -> bool {
        true
    }

    pub fn prism(&self) -> &Prism {
        &self.prism
    }

    /// Look up entries matching a sequence of syllable strings.
    pub fn lookup_words(&self, code: &[&str], predictive: bool) -> Vec<DictEntry> {
        self.table.lookup_str(code, predictive)
    }

    /// Look up entries matching a sequence of syllable ids.
    pub fn lookup_code(&self, code: &[SyllableId], predictive: bool) -> Vec<DictEntry> {
        self.table.lookup(code, predictive)
    }

    /// Decode syllable ids back to syllable strings.
    pub fn decode(&self, code: &[SyllableId], result: &mut Vec<String>) -> bool {
        for &id in code {
            match self.table.get_syllable_by_id(id) {
                Some(s) => result.push(s.to_string()),
                None => return false,
            }
        }
        true
    }
}

thread_local! {
    static DICTIONARY_STORE: RefCell<HashMap<String, Rc<Dictionary>>> =
        RefCell::new(HashMap::new());
}

/// Register a dictionary under a name (called during setup/deployment).
pub fn register_dictionary(name: &str, dictionary: Rc<Dictionary>) {
    DICTIONARY_STORE.with(|store| {
        store.borrow_mut().insert(name.to_string(), dictionary);
    });
}

pub fn find_dictionary(name: &str) -> Option<Rc<Dictionary>> {
    DICTIONARY_STORE.with(|store| store.borrow().get(name).cloned())
}
