// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/config/config_types.* and config_data.* (the read-side
// data model used by the engine). YAML loading/emitting is deferred to a later
// milestone; the tree can be constructed programmatically or by a future loader.

use std::collections::BTreeMap;

/// A node in a config tree.
#[derive(Debug, Clone, PartialEq)]
pub enum ConfigItem {
    Null,
    Scalar(String),
    List(Vec<ConfigItem>),
    Map(BTreeMap<String, ConfigItem>),
}

impl ConfigItem {
    pub fn is_null(&self) -> bool {
        matches!(self, ConfigItem::Null)
    }
    pub fn is_scalar(&self) -> bool {
        matches!(self, ConfigItem::Scalar(_))
    }
    pub fn is_list(&self) -> bool {
        matches!(self, ConfigItem::List(_))
    }
    pub fn is_map(&self) -> bool {
        matches!(self, ConfigItem::Map(_))
    }

    pub fn as_str(&self) -> Option<&str> {
        match self {
            ConfigItem::Scalar(s) => Some(s),
            _ => None,
        }
    }
    pub fn as_list(&self) -> Option<&[ConfigItem]> {
        match self {
            ConfigItem::List(l) => Some(l),
            _ => None,
        }
    }
    pub fn as_map(&self) -> Option<&BTreeMap<String, ConfigItem>> {
        match self {
            ConfigItem::Map(m) => Some(m),
            _ => None,
        }
    }

    /// Parse a scalar as bool (accepts case-insensitive "true"/"false").
    pub fn get_bool(&self) -> Option<bool> {
        let s = self.as_str()?;
        match s.to_ascii_lowercase().as_str() {
            "true" => Some(true),
            "false" => Some(false),
            _ => None,
        }
    }

    /// Parse a scalar as int (decimal, or hexadecimal with 0x prefix).
    pub fn get_int(&self) -> Option<i32> {
        let s = self.as_str()?;
        if let Some(hex) = s.strip_prefix("0x") {
            return i32::from_str_radix(hex, 16).ok();
        }
        s.parse::<i32>().ok()
    }

    pub fn get_double(&self) -> Option<f64> {
        let s = self.as_str()?;
        s.parse::<f64>().ok()
    }

    pub fn get_string(&self) -> Option<&str> {
        self.as_str()
    }
}

impl Default for ConfigItem {
    fn default() -> Self {
        ConfigItem::Null
    }
}

impl From<&str> for ConfigItem {
    fn from(s: &str) -> Self {
        ConfigItem::Scalar(s.to_string())
    }
}

impl From<String> for ConfigItem {
    fn from(s: String) -> Self {
        ConfigItem::Scalar(s)
    }
}

impl From<bool> for ConfigItem {
    fn from(b: bool) -> Self {
        ConfigItem::Scalar(if b { "true" } else { "false" }.to_string())
    }
}

impl From<i32> for ConfigItem {
    fn from(i: i32) -> Self {
        ConfigItem::Scalar(i.to_string())
    }
}

/// An in-memory config document.
#[derive(Debug, Clone, Default)]
pub struct Config {
    pub root: ConfigItem,
}

impl Config {
    pub fn new(root: ConfigItem) -> Self {
        Self { root }
    }

    /// Parse a YAML document into a config tree.
    pub fn from_yaml(input: &str) -> Self {
        Self {
            root: crate::yaml::parse_yaml(input),
        }
    }

    /// Traverse a slash-separated path; `@N` indexes into lists.
    pub fn get(&self, path: &str) -> Option<&ConfigItem> {
        let mut current = &self.root;
        for key in split_path(path) {
            if let Some(index) = parse_list_index(&key) {
                current = current.as_list()?.get(index)?;
            } else {
                current = current.as_map()?.get(&key)?;
            }
        }
        Some(current)
    }

    pub fn get_list(&self, path: &str) -> Option<&[ConfigItem]> {
        self.get(path).and_then(|item| item.as_list())
    }

    pub fn get_map(&self, path: &str) -> Option<&BTreeMap<String, ConfigItem>> {
        self.get(path).and_then(|item| item.as_map())
    }

    pub fn get_value(&self, path: &str) -> Option<&str> {
        self.get(path).and_then(|item| item.as_str())
    }

    pub fn get_string(&self, path: &str) -> Option<&str> {
        self.get_value(path)
    }

    pub fn get_bool(&self, path: &str) -> Option<bool> {
        self.get(path).and_then(|item| item.get_bool())
    }

    pub fn get_int(&self, path: &str) -> Option<i32> {
        self.get(path).and_then(|item| item.get_int())
    }

    pub fn get_double(&self, path: &str) -> Option<f64> {
        self.get(path).and_then(|item| item.get_double())
    }

    /// Set a value at a slash-separated path, creating intermediate maps/lists.
    pub fn set(&mut self, path: &str, item: ConfigItem) {
        let keys = split_path(path);
        if keys.is_empty() {
            self.root = item;
            return;
        }
        let mut current = &mut self.root;
        for (i, key) in keys.iter().enumerate() {
            let is_last = i == keys.len() - 1;
            if let Some(index) = parse_list_index(key) {
                if !current.is_list() {
                    *current = ConfigItem::List(Vec::new());
                }
                let list = match current {
                    ConfigItem::List(l) => l,
                    _ => unreachable!(),
                };
                while list.len() <= index {
                    list.push(ConfigItem::Null);
                }
                if is_last {
                    list[index] = item;
                    return;
                }
                current = &mut list[index];
            } else {
                if !current.is_map() {
                    *current = ConfigItem::Map(BTreeMap::new());
                }
                let map = match current {
                    ConfigItem::Map(m) => m,
                    _ => unreachable!(),
                };
                if is_last {
                    map.insert(key.clone(), item);
                    return;
                }
                current = map.entry(key.clone()).or_insert(ConfigItem::Map(BTreeMap::new()));
            }
        }
    }

    pub fn set_string(&mut self, path: &str, value: &str) {
        self.set(path, ConfigItem::Scalar(value.to_string()));
    }

    pub fn set_bool(&mut self, path: &str, value: bool) {
        self.set(path, ConfigItem::from(value));
    }

    pub fn set_int(&mut self, path: &str, value: i32) {
        self.set(path, ConfigItem::from(value));
    }

    pub fn set_double(&mut self, path: &str, value: f64) {
        self.set(path, ConfigItem::Scalar(value.to_string()));
    }
}

/// Split a path on '/', trimming leading separators.
pub fn split_path(path: &str) -> Vec<String> {
    path.trim_start_matches('/')
        .split('/')
        .filter(|s| !s.is_empty())
        .map(|s| s.to_string())
        .collect()
}

/// Parse a list-item reference key `@N` (returns None if not a list index).
pub fn parse_list_index(key: &str) -> Option<usize> {
    if key.len() > 1 && key.starts_with('@') {
        key[1..].parse::<usize>().ok()
    } else {
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sample() -> Config {
        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(
                [(
                    "processors".to_string(),
                    ConfigItem::List(vec![
                        ConfigItem::Scalar("ascii_composer".to_string()),
                        ConfigItem::Scalar("speller".to_string()),
                    ]),
                )]
                .into_iter()
                .collect(),
            ),
        );
        root.insert("menu".to_string(), ConfigItem::Scalar("5".to_string()));
        Config::new(ConfigItem::Map(root))
    }

    #[test]
    fn traverse_and_get_list() {
        let c = sample();
        let list = c.get_list("engine/processors").unwrap();
        assert_eq!(list.len(), 2);
        assert_eq!(list[0].as_str(), Some("ascii_composer"));
        assert_eq!(c.get_value("menu"), Some("5"));
        assert_eq!(c.get_int("menu"), Some(5));
    }

    #[test]
    fn list_index_reference() {
        let mut root = BTreeMap::new();
        root.insert(
            "switches".to_string(),
            ConfigItem::List(vec![ConfigItem::Scalar("a".into()), ConfigItem::Scalar("b".into())]),
        );
        let c = Config::new(ConfigItem::Map(root));
        assert_eq!(c.get_value("switches/@0"), Some("a"));
        assert_eq!(c.get_value("switches/@1"), Some("b"));
        assert_eq!(c.get("switches/@5"), None);
    }

    #[test]
    fn set_creates_nested_structure() {
        let mut c = Config::new(ConfigItem::Map(BTreeMap::new()));
        c.set_string("menu/page_size", "10");
        assert_eq!(c.get_string("menu/page_size"), Some("10"));
        c.set_int("switches/@0/reset", 1);
        assert_eq!(c.get_int("switches/@0/reset"), Some(1));
        c.set_bool("flag", true);
        assert_eq!(c.get_bool("flag"), Some(true));
    }
}
