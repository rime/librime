// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/gear/ to Rust — pipeline component implementations.

pub mod abc_segmentor;
pub mod echo_translator;
pub mod script_translator;
pub mod selector;
pub mod speller;
pub mod table_translator;

#[cfg(test)]
mod tests {
    use super::abc_segmentor::AbcSegmentor;
    use super::echo_translator::EchoTranslator;
    use super::script_translator::ScriptTranslator;
    use super::selector::Selector;
    use super::speller::Speller;
    use super::table_translator::TableTranslator;
    use crate::component::{register_processor, register_segmentor, register_translator};
    use crate::config::{Config, ConfigItem};
    use crate::dict::dictionary::{register_dictionary, Dictionary};
    use crate::dict::table::Table;
    use crate::engine::Engine;
    use crate::key_event::KeyEvent;
    use crate::schema::Schema;
    use std::collections::BTreeMap;
    use std::rc::Rc;

    fn pipeline_schema() -> Schema {
        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(BTreeMap::from([
                (
                    "segmentors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("abc_segmentor".to_string())]),
                ),
                (
                    "translators".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("echo_translator".to_string())]),
                ),
            ])),
        );
        root.insert(
            "speller".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "alphabet".to_string(),
                ConfigItem::Scalar("abcdefghijklmnopqrstuvwxyz".to_string()),
            )])),
        );
        Schema::new("test", Rc::new(Config::new(ConfigItem::Map(root))))
    }

    #[test]
    fn abc_segmentor_echo_pipeline() {
        register_segmentor("abc_segmentor", |ticket| {
            Some(Box::new(AbcSegmentor::new(ticket)))
        });
        register_translator("echo_translator", |ticket| {
            Some(Box::new(EchoTranslator::new(ticket)))
        });

        let engine = Engine::new(pipeline_schema());
        engine.context().borrow_mut().push_input_str("nihao");

        let ctx = engine.context().borrow();
        let cand = ctx.get_selected_candidate();
        assert!(cand.is_some());
        assert_eq!(cand.unwrap().text(), "nihao");

        let comp = ctx.composition();
        assert_eq!(comp.len(), 1);
        assert!(comp[0].has_tag("abc"));
        assert_eq!(comp[0].start, 0);
        assert_eq!(comp[0].end, 5);
    }

    #[test]
    fn table_translator_looks_up_dictionary() {
        // build an in-memory dictionary
        let mut table = Table::new();
        table.add_entry(&["ni"], "你", 1.0);
        table.add_entry(&["hao"], "好", 1.0);
        table.add_entry(&["ni", "hao"], "你好", 1.5);
        register_dictionary("luna_pinyin", Rc::new(Dictionary::new("luna_pinyin", Rc::new(table))));

        register_segmentor("abc_segmentor", |ticket| {
            Some(Box::new(AbcSegmentor::new(ticket)))
        });
        register_translator("table_translator", |ticket| {
            Some(Box::new(TableTranslator::new(ticket)))
        });

        // schema: abc_segmentor + table_translator, delimiter = space
        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(BTreeMap::from([
                (
                    "segmentors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("abc_segmentor".to_string())]),
                ),
                (
                    "translators".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("table_translator".to_string())]),
                ),
            ])),
        );
        root.insert(
            "speller".to_string(),
            ConfigItem::Map(BTreeMap::from([
                ("alphabet".to_string(), ConfigItem::Scalar("abcdefghijklmnopqrstuvwxyz".to_string())),
                ("delimiter".to_string(), ConfigItem::Scalar(" ".to_string())),
            ])),
        );
        root.insert(
            "translator".to_string(),
            ConfigItem::Map(BTreeMap::from([
                ("dictionary".to_string(), ConfigItem::Scalar("luna_pinyin".to_string())),
                ("delimiter".to_string(), ConfigItem::Scalar(" ".to_string())),
            ])),
        );
        let schema = Schema::new("test", Rc::new(Config::new(ConfigItem::Map(root))));

        let engine = Engine::new(schema);
        engine.context().borrow_mut().push_input_str("ni hao");

        let cand = engine.context().borrow().get_selected_candidate();
        assert!(cand.is_some());
        assert_eq!(cand.unwrap().text(), "你好");
    }

    #[test]
    fn script_translator_syllabifies_undelimited_pinyin() {
        register_segmentor("abc_segmentor", |ticket| {
            Some(Box::new(AbcSegmentor::new(ticket)))
        });
        register_translator("script_translator", |ticket| {
            Some(Box::new(ScriptTranslator::new(ticket)))
        });

        let mut table = Table::new();
        table.add_entry(&["ni"], "你", 1.0);
        table.add_entry(&["hao"], "好", 1.0);
        table.add_entry(&["ni", "hao"], "你好", 1.5);
        register_dictionary("pinyin", Rc::new(Dictionary::new("pinyin", Rc::new(table))));

        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(BTreeMap::from([
                (
                    "segmentors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("abc_segmentor".to_string())]),
                ),
                (
                    "translators".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("script_translator".to_string())]),
                ),
            ])),
        );
        root.insert(
            "speller".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "alphabet".to_string(),
                ConfigItem::Scalar("abcdefghijklmnopqrstuvwxyz".to_string()),
            )])),
        );
        root.insert(
            "translator".to_string(),
            ConfigItem::Map(BTreeMap::from([
                ("dictionary".to_string(), ConfigItem::Scalar("pinyin".to_string())),
                ("delimiters".to_string(), ConfigItem::Scalar("'".to_string())),
            ])),
        );
        let schema = Schema::new("pinyin", Rc::new(Config::new(ConfigItem::Map(root))));

        let engine = Engine::new(schema);
        // no delimiter: "nihao" is syllabified into ni + hao by the prism
        engine.context().borrow_mut().push_input_str("nihao");

        let cand = engine.context().borrow().get_selected_candidate();
        assert!(cand.is_some());
        assert_eq!(cand.unwrap().text(), "你好");
    }

    #[test]
    fn full_key_typing_pipeline() {
        register_processor("speller", |ticket| Some(Box::new(Speller::new(ticket))));
        register_segmentor("abc_segmentor", |ticket| {
            Some(Box::new(AbcSegmentor::new(ticket)))
        });
        register_translator("script_translator", |ticket| {
            Some(Box::new(ScriptTranslator::new(ticket)))
        });

        let mut table = Table::new();
        table.add_entry(&["ni"], "你", 1.0);
        table.add_entry(&["hao"], "好", 1.0);
        table.add_entry(&["ni", "hao"], "你好", 1.5);
        register_dictionary("pinyin", Rc::new(Dictionary::new("pinyin", Rc::new(table))));

        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(BTreeMap::from([
                (
                    "processors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("speller".to_string())]),
                ),
                (
                    "segmentors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("abc_segmentor".to_string())]),
                ),
                (
                    "translators".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("script_translator".to_string())]),
                ),
            ])),
        );
        root.insert(
            "speller".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "alphabet".to_string(),
                ConfigItem::Scalar("abcdefghijklmnopqrstuvwxyz".to_string()),
            )])),
        );
        root.insert(
            "translator".to_string(),
            ConfigItem::Map(BTreeMap::from([
                ("dictionary".to_string(), ConfigItem::Scalar("pinyin".to_string())),
                ("delimiters".to_string(), ConfigItem::Scalar("'".to_string())),
            ])),
        );
        let schema = Schema::new("pinyin", Rc::new(Config::new(ConfigItem::Map(root))));

        let engine = Engine::new(schema);

        // type "nihao" through ProcessKey (speller pushes each letter)
        for c in "nihao".chars() {
            let key = KeyEvent::with_keycode(c as i32, 0);
            assert!(engine.process_key(&key), "key '{}' should be accepted", c);
        }

        let cand = engine.context().borrow().get_selected_candidate();
        assert!(cand.is_some());
        assert_eq!(cand.unwrap().text(), "你好");
    }

    #[test]
    fn selector_navigates_candidates() {
        use crate::key_table::XK_DOWN;

        register_processor("speller", |ticket| Some(Box::new(Speller::new(ticket))));
        register_processor("selector", |ticket| Some(Box::new(Selector::new(ticket))));
        register_segmentor("abc_segmentor", |ticket| {
            Some(Box::new(AbcSegmentor::new(ticket)))
        });
        register_translator("script_translator", |ticket| {
            Some(Box::new(ScriptTranslator::new(ticket)))
        });

        let mut table = Table::new();
        table.add_entry(&["hao"], "好", 3.0);
        table.add_entry(&["hao"], "号", 2.0);
        table.add_entry(&["hao"], "毫", 1.0);
        register_dictionary("pinyin", Rc::new(Dictionary::new("pinyin", Rc::new(table))));

        let mut root = BTreeMap::new();
        root.insert(
            "engine".to_string(),
            ConfigItem::Map(BTreeMap::from([
                (
                    "processors".to_string(),
                    ConfigItem::List(vec![
                        ConfigItem::Scalar("speller".to_string()),
                        ConfigItem::Scalar("selector".to_string()),
                    ]),
                ),
                (
                    "segmentors".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("abc_segmentor".to_string())]),
                ),
                (
                    "translators".to_string(),
                    ConfigItem::List(vec![ConfigItem::Scalar("script_translator".to_string())]),
                ),
            ])),
        );
        root.insert(
            "speller".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "alphabet".to_string(),
                ConfigItem::Scalar("abcdefghijklmnopqrstuvwxyz".to_string()),
            )])),
        );
        root.insert(
            "translator".to_string(),
            ConfigItem::Map(BTreeMap::from([
                ("dictionary".to_string(), ConfigItem::Scalar("pinyin".to_string())),
                ("delimiters".to_string(), ConfigItem::Scalar("'".to_string())),
                ("max_homophones".to_string(), ConfigItem::Scalar("5".to_string())),
            ])),
        );
        root.insert(
            "menu".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "page_size".to_string(),
                ConfigItem::Scalar("5".to_string()),
            )])),
        );
        let schema = Schema::new("pinyin", Rc::new(Config::new(ConfigItem::Map(root))));
        let engine = Engine::new(schema);

        for c in "hao".chars() {
            assert!(engine.process_key(&KeyEvent::with_keycode(c as i32, 0)));
        }
        // three homophones; the first is selected by default
        assert_eq!(engine.context().borrow().get_selected_candidate().unwrap().text(), "好");

        // Down arrow highlights the next candidate
        assert!(engine.process_key(&KeyEvent::with_keycode(XK_DOWN, 0)));
        assert_eq!(engine.context().borrow().get_selected_candidate().unwrap().text(), "号");
    }
}
