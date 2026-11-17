// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/schema.h and src/rime/schema.cc to Rust.

use std::collections::BTreeMap;
use std::rc::Rc;

use crate::config::{Config, ConfigItem};

/// A Rime input schema (its config plus frequently used items).
#[derive(Debug)]
pub struct Schema {
    schema_id: String,
    schema_name: String,
    config: Rc<Config>,
    page_size: usize,
    page_down_cycle: bool,
    select_keys: String,
}

impl Schema {
    pub fn new(schema_id: &str, config: Rc<Config>) -> Self {
        let schema_name = config
            .get_string("schema/name")
            .map(|s| s.to_string())
            .unwrap_or_else(|| schema_id.to_string());
        let mut page_size = config.get_int("menu/page_size").unwrap_or(5) as usize;
        if page_size < 1 {
            page_size = 5;
        }
        let select_keys = config
            .get_string("menu/alternative_select_keys")
            .map(|s| s.to_string())
            .unwrap_or_default();
        let page_down_cycle = config.get_bool("menu/page_down_cycle").unwrap_or(false);
        Self {
            schema_id: schema_id.to_string(),
            schema_name,
            config,
            page_size,
            page_down_cycle,
            select_keys,
        }
    }

    /// An empty schema with id ".default".
    pub fn default_schema() -> Self {
        Self::new(".default", Rc::new(Config::new(ConfigItem::Map(BTreeMap::new()))))
    }

    pub fn schema_id(&self) -> &str {
        &self.schema_id
    }
    pub fn schema_name(&self) -> &str {
        &self.schema_name
    }
    pub fn config(&self) -> &Rc<Config> {
        &self.config
    }
    pub fn page_size(&self) -> usize {
        self.page_size
    }
    pub fn page_down_cycle(&self) -> bool {
        self.page_down_cycle
    }
    pub fn select_keys(&self) -> &str {
        &self.select_keys
    }
    pub fn set_select_keys(&mut self, keys: &str) {
        self.select_keys = keys.to_string();
    }
}
