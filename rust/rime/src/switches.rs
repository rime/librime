// Copyright RIME Developers
// Distributed under the BSD License
//
// Minimal port of src/rime/switches.h and switches.cc — the subset the engine
// needs to reset switch options on schema load.

use crate::config::{Config, ConfigItem};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SwitchType {
    ToggleOption,
    RadioGroup,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FindResult {
    Continue,
    Found,
}

#[derive(Debug, Clone)]
pub struct SwitchOption {
    pub type_: SwitchType,
    pub option_name: String,
    /// Reset state value on initialize; -1 if unspecified.
    pub reset_value: i32,
    pub switch_index: usize,
    pub option_index: usize,
}

pub struct Switches<'a> {
    config: &'a Config,
}

impl<'a> Switches<'a> {
    pub fn new(config: &'a Config) -> Self {
        Self { config }
    }

    fn reset_value(map: &std::collections::BTreeMap<String, ConfigItem>) -> i32 {
        map.get("reset").and_then(|v| v.get_int()).unwrap_or(-1)
    }

    pub fn find_option(
        &self,
        mut callback: impl FnMut(&SwitchOption) -> FindResult,
    ) -> Option<SwitchOption> {
        let Some(switches) = self.config.get_list("switches") else {
            return None;
        };
        for (switch_index, item) in switches.iter().enumerate() {
            let Some(map) = item.as_map() else { continue };
            if let Some(name) = map.get("name").and_then(|v| v.as_str()) {
                let option = SwitchOption {
                    type_: SwitchType::ToggleOption,
                    option_name: name.to_string(),
                    reset_value: Self::reset_value(map),
                    switch_index,
                    option_index: 0,
                };
                if callback(&option) == FindResult::Found {
                    return Some(option);
                }
            } else if let Some(options) = map.get("options").and_then(|v| v.as_list()) {
                for (option_index, opt) in options.iter().enumerate() {
                    if let Some(name) = opt.as_str() {
                        let option = SwitchOption {
                            type_: SwitchType::RadioGroup,
                            option_name: name.to_string(),
                            reset_value: Self::reset_value(map),
                            switch_index,
                            option_index,
                        };
                        if callback(&option) == FindResult::Found {
                            return Some(option);
                        }
                    }
                }
            }
        }
        None
    }
}
