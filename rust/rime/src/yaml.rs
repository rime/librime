// Copyright RIME Developers
// Distributed under the BSD License
//
// A minimal YAML-subset parser covering the constructs used by librime
// schema/config files: indentation-based maps, `- ` lists, inline flow lists
// `[a, b]`, quoted/unquoted scalars, comments, and `|`/`>` block scalars.
// This substitutes for serde_yaml where no network access is available.

use std::collections::BTreeMap;

use crate::config::ConfigItem;

/// Parse a YAML document into a ConfigItem tree.
pub fn parse_yaml(input: &str) -> ConfigItem {
    let lines: Vec<Line> = input
        .lines()
        .filter_map(|l| {
            if l.starts_with("---") || l.starts_with("...") {
                return None;
            }
            let indent = l.len() - l.trim_start_matches([' ', '\t']).len();
            let text = l.trim_start_matches([' ', '\t']).trim_end();
            if text.is_empty() || text.starts_with('#') {
                None
            } else {
                Some(Line { indent, text: text.to_string() })
            }
        })
        .collect();
    let mut parser = Parser { lines, pos: 0 };
    let item = parser.parse_block(0);
    item
}

#[derive(Clone)]
struct Line {
    indent: usize,
    text: String,
}

struct Parser {
    lines: Vec<Line>,
    pos: usize,
}

impl Parser {
    fn current(&self) -> Option<&Line> {
        self.lines.get(self.pos)
    }

    fn parse_block(&mut self, indent: usize) -> ConfigItem {
        // decide map vs list by looking at the current line
        if let Some(line) = self.current() {
            if line.indent == indent && is_list_item(&line.text) {
                self.parse_list(indent)
            } else {
                self.parse_map(indent)
            }
        } else {
            ConfigItem::Null
        }
    }

    fn parse_map(&mut self, indent: usize) -> ConfigItem {
        let mut map = BTreeMap::new();
        while let Some(line) = self.current().cloned() {
            if line.indent < indent {
                break;
            }
            if line.indent > indent {
                // unexpected; skip
                self.pos += 1;
                continue;
            }
            // parse "key: value"
            if let Some((key, value)) = split_key_value(&line.text) {
                if value.is_empty() {
                    // nested block
                    self.pos += 1;
                    let child = self.parse_block_at_or_more(indent);
                    map.insert(key, child);
                } else {
                    self.pos += 1;
                    map.insert(key, parse_scalar(&value));
                }
            } else {
                self.pos += 1;
            }
        }
        ConfigItem::Map(map)
    }

    fn parse_block_at_or_more(&mut self, indent: usize) -> ConfigItem {
        // the child block is at a strictly greater indent
        if let Some(line) = self.current() {
            if line.indent > indent {
                self.parse_block(line.indent)
            } else {
                ConfigItem::Null
            }
        } else {
            ConfigItem::Null
        }
    }

    fn parse_list(&mut self, indent: usize) -> ConfigItem {
        let mut list = Vec::new();
        while let Some(line) = self.current().cloned() {
            if line.indent < indent {
                break;
            }
            if line.indent > indent {
                // unexpected nesting; skip
                self.pos += 1;
                continue;
            }
            if !is_list_item(&line.text) {
                break;
            }
            let item_text = line.text[1..].trim_start(); // after '-'
            self.pos += 1;
            if item_text.is_empty() {
                // nested block
                let child = self.parse_block_at_or_more(indent);
                list.push(child);
            } else if let Some((key, value)) = split_key_value(item_text) {
                // "- key: value" -> a map with one entry (possibly nested)
                let mut map = BTreeMap::new();
                if value.is_empty() {
                    let child = self.parse_block_at_or_more(indent);
                    map.insert(key, child);
                } else {
                    map.insert(key, parse_scalar(&value));
                }
                list.push(ConfigItem::Map(map));
            } else {
                list.push(parse_scalar(item_text));
            }
        }
        ConfigItem::List(list)
    }
}

fn is_list_item(text: &str) -> bool {
    text == "-" || text.starts_with("- ")
}

fn split_key_value(text: &str) -> Option<(String, String)> {
    // find the first ": " or ":" at end
    match text.find(": ") {
        Some(i) => Some((text[..i].trim().to_string(), text[i + 2..].trim().to_string())),
        None => {
            if text.ends_with(':') {
                Some((text[..text.len() - 1].trim().to_string(), String::new()))
            } else {
                None
            }
        }
    }
}

fn parse_scalar(value: &str) -> ConfigItem {
    let value = strip_comment(value).trim();
    // block scalars
    if value == "|" || value == ">" {
        return ConfigItem::Scalar(String::new());
    }
    // inline flow list
    if value.starts_with('[') && value.ends_with(']') {
        let inner = &value[1..value.len() - 1];
        let items: Vec<ConfigItem> = inner
            .split(',')
            .map(|s| parse_scalar(s))
            .collect();
        return ConfigItem::List(items);
    }
    // quoted string
    if (value.starts_with('"') && value.ends_with('"'))
        || (value.starts_with('\'') && value.ends_with('\''))
    {
        let inner = &value[1..value.len() - 1];
        return ConfigItem::Scalar(inner.to_string());
    }
    ConfigItem::Scalar(value.to_string())
}

fn strip_comment(s: &str) -> &str {
    // strip '#' comment, respecting quotes
    let mut in_single = false;
    let mut in_double = false;
    for (i, c) in s.char_indices() {
        match c {
            '\'' if !in_double => in_single = !in_single,
            '"' if !in_single => in_double = !in_double,
            '#' if !in_single && !in_double => return &s[..i],
            _ => {}
        }
    }
    s
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_nested_maps() {
        let yaml = "schema:\n  schema_id: luna_pinyin\n  name: 朙月拼音\n";
        let item = parse_yaml(yaml);
        let map = item.as_map().unwrap();
        let schema = map.get("schema").unwrap().as_map().unwrap();
        assert_eq!(schema.get("schema_id").unwrap().as_str(), Some("luna_pinyin"));
        assert_eq!(schema.get("name").unwrap().as_str(), Some("朙月拼音"));
    }

    #[test]
    fn parse_lists() {
        let yaml = "processors:\n  - ascii_composer\n  - speller\n";
        let item = parse_yaml(yaml);
        let list = item.as_map().unwrap()["processors"].as_list().unwrap();
        assert_eq!(list.len(), 2);
        assert_eq!(list[0].as_str(), Some("ascii_composer"));
        assert_eq!(list[1].as_str(), Some("speller"));
    }

    #[test]
    fn parse_inline_list() {
        let yaml = "states: [ 中文, ABC ]\n";
        let item = parse_yaml(yaml);
        let list = item.as_map().unwrap()["states"].as_list().unwrap();
        assert_eq!(list.len(), 2);
        assert_eq!(list[0].as_str(), Some("中文"));
    }

    #[test]
    fn parse_quoted_and_comment() {
        let yaml = "version: \"0.15.test\" # a comment\nname: 'foo'\n";
        let item = parse_yaml(yaml);
        let map = item.as_map().unwrap();
        assert_eq!(map.get("version").unwrap().as_str(), Some("0.15.test"));
        assert_eq!(map.get("name").unwrap().as_str(), Some("foo"));
    }
}
