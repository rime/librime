// Integration tests: load the real minimal schema/dictionary files and run the
// full key-typing pipeline end to end.

use std::path::Path;
use std::rc::Rc;

use rime::component::{register_processor, register_segmentor, register_translator};
use rime::config::Config;
use rime::dict::dictionary::{register_dictionary, Dictionary};
use rime::dict::table::Table;
use rime::engine::Engine;
use rime::gear::abc_segmentor::AbcSegmentor;
use rime::gear::script_translator::ScriptTranslator;
use rime::gear::speller::Speller;
use rime::key_event::KeyEvent;
use rime::schema::Schema;

fn data_path(name: &str) -> String {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../data/minimal")
        .join(name)
        .to_string_lossy()
        .into_owned()
}

#[test]
fn loads_real_dict_and_schema() {
    let dict_yaml = std::fs::read_to_string(data_path("luna_pinyin.dict.yaml")).unwrap();
    let table = Table::from_dict_yaml(&dict_yaml);
    assert!(table.get_syllable_id("ni").is_some());
    assert!(table.lookup_str(&["ni"], false).iter().any(|e| e.text == "你"));
    assert!(table.lookup_str(&["ni", "nan"], false).iter().any(|e| e.text == "呢喃"));

    let schema_yaml = std::fs::read_to_string(data_path("luna_pinyin.schema.yaml")).unwrap();
    let config = Config::from_yaml(&schema_yaml);
    assert_eq!(config.get_string("schema/schema_id"), Some("luna_pinyin"));
    assert_eq!(config.get_string("speller/alphabet"), Some("zyxwvutsrqponmlkjihgfedcba"));
}

#[test]
fn full_pipeline_with_real_dict() {
    register_processor("speller", |t| Some(Box::new(Speller::new(t))));
    register_segmentor("abc_segmentor", |t| Some(Box::new(AbcSegmentor::new(t))));
    register_translator("script_translator", |t| Some(Box::new(ScriptTranslator::new(t))));

    let dict_yaml = std::fs::read_to_string(data_path("luna_pinyin.dict.yaml")).unwrap();
    let table = Rc::new(Table::from_dict_yaml(&dict_yaml));
    register_dictionary("luna_pinyin", Rc::new(Dictionary::new("luna_pinyin", table)));

    // a minimal schema using the components implemented so far
    let schema_yaml = r#"
schema:
  schema_id: luna_pinyin
  name: 朙月拼音
speller:
  alphabet: zyxwvutsrqponmlkjihgfedcba
  delimiter: " '"
translator:
  dictionary: luna_pinyin
  delimiters: "'"
engine:
  processors:
    - speller
  segmentors:
    - abc_segmentor
  translators:
    - script_translator
"#;
    let config = Rc::new(Config::from_yaml(schema_yaml));
    let schema = Schema::new("luna_pinyin", config);
    let engine = Engine::new(schema);

    // type "ninan" -> syllabified ni + nan -> 呢喃
    for c in "ninan".chars() {
        assert!(engine.process_key(&KeyEvent::with_keycode(c as i32, 0)));
    }
    let cand = engine.context().borrow().get_selected_candidate();
    assert!(cand.is_some());
    assert_eq!(cand.unwrap().text(), "呢喃");
}
