// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/engine.h and src/rime/engine.cc (the concrete engine) to Rust.
//
// The engine is shared as `Rc<Engine>`; its per-component and sink state uses
// interior mutability (RefCell/Cell) so that the synchronous notifier callbacks
// (compose/on_commit/on_option_update) may re-enter the engine without
// re-borrowing the whole object.

use std::cell::{Cell, RefCell};
use std::rc::Rc;

use crate::commit_history::CommitRecord;
use crate::component::{
    require_filter, require_processor, require_segmentor, require_translator,
};
use crate::config::ConfigItem;
use crate::context::{Context, ContextRc};
use crate::filter::Filter;
use crate::formatter::Formatter;
use crate::key_event::KeyEvent;
use crate::menu::Menu;
use crate::processor::{ProcessResult, Processor};
use crate::schema::Schema;
use crate::segmentation::{SegmentStatus, Segmentation};
use crate::segmentor::Segmentor;
use crate::switches::{FindResult, Switches, SwitchType};
use crate::ticket::Ticket;
use crate::translator::Translator;
use crate::util::substr;

pub type EngineRef = Rc<Engine>;

/// A component plus its enablement condition.
struct ComponentEntry<T: ?Sized> {
    component: RefCell<Box<T>>,
    condition: Option<Condition>,
    enabled: Cell<bool>,
}

#[derive(Debug, Clone)]
enum Condition {
    Option { name: String, expected: bool },
    AllOf(Vec<Condition>),
    AnyOf(Vec<Condition>),
    NoneOf(Vec<Condition>),
}

fn evaluate_condition(condition: &Condition, ctx: &Context) -> bool {
    match condition {
        Condition::Option { name, expected } => ctx.get_option(name) == *expected,
        Condition::AllOf(children) => children.iter().all(|c| evaluate_condition(c, ctx)),
        Condition::AnyOf(children) => children.iter().any(|c| evaluate_condition(c, ctx)),
        Condition::NoneOf(children) => children.iter().all(|c| !evaluate_condition(c, ctx)),
    }
}

fn update_conditions<T: ?Sized>(components: &[ComponentEntry<T>], ctx: &Context) {
    for entry in components {
        entry.enabled.set(match &entry.condition {
            Some(c) => evaluate_condition(c, ctx),
            None => true,
        });
    }
}

/// Parse a component prescription: a scalar `klass`, or a map `{name, when}`.
fn parse_component_spec(item: &ConfigItem) -> Option<(String, Option<Condition>)> {
    if let Some(value) = item.as_str() {
        return Some((value.to_string(), None));
    }
    let map = item.as_map()?;
    for key in map.keys() {
        if key != "name" && key != "when" {
            return None;
        }
    }
    let name = map.get("name")?.as_str()?;
    if name.is_empty() {
        return None;
    }
    let condition = if let Some(when_node) = map.get("when") {
        Some(parse_condition(when_node)?)
    } else {
        None
    };
    Some((name.to_string(), condition))
}

fn parse_condition(item: &ConfigItem) -> Option<Condition> {
    if let Some(value) = item.as_str() {
        let mut option = value.to_string();
        if option.is_empty() {
            return None;
        }
        let mut expected = true;
        if option.starts_with('!') {
            expected = false;
            option.remove(0);
        }
        if option.is_empty() {
            return None;
        }
        return Some(Condition::Option {
            name: option,
            expected,
        });
    }
    let map = item.as_map()?;
    if map.contains_key("option") {
        if map.len() != 2 || !map.contains_key("value") {
            return None;
        }
        let option = map.get("option")?.as_str()?;
        if option.is_empty() {
            return None;
        }
        let expected = map.get("value")?.get_bool()?;
        return Some(Condition::Option {
            name: option.to_string(),
            expected,
        });
    }
    let (operation, kind) = if map.contains_key("all_of") {
        ("all_of", 0)
    } else if map.contains_key("any_of") {
        ("any_of", 1)
    } else if map.contains_key("none_of") {
        ("none_of", 2)
    } else {
        return None;
    };
    if map.len() != 1 {
        return None;
    }
    let list = map.get(operation)?.as_list()?;
    if list.is_empty() {
        return None;
    }
    let children: Vec<Condition> = list
        .iter()
        .map(parse_condition)
        .collect::<Option<Vec<_>>>()?;
    match kind {
        0 => Some(Condition::AllOf(children)),
        1 => Some(Condition::AnyOf(children)),
        _ => Some(Condition::NoneOf(children)),
    }
}

fn split_prescription(prescription: &str, default_ns: &str) -> (String, String) {
    match prescription.find('@') {
        Some(separator) => (
            prescription[..separator].to_string(),
            prescription[separator + 1..].to_string(),
        ),
        None => (prescription.to_string(), default_ns.to_string()),
    }
}

macro_rules! create_components {
    ($self:ident, $config_key:expr, $component_type:expr, $require:path, $target:ident) => {{
        let context = $self.context.clone();
        let schema_config = $self.schema.config().clone();
        if let Some(list) = schema_config.get_list($config_key) {
            let items = list.to_vec();
            for item in &items {
                let Some((prescription, condition)) = parse_component_spec(item) else {
                    continue;
                };
                let (klass, name_space) = split_prescription(&prescription, $component_type);
                let Some(factory) = $require(&klass) else {
                    continue;
                };
                let ticket = Ticket {
                    context: context.clone(),
                    schema_config: schema_config.clone(),
                    name_space,
                    klass: klass.clone(),
                };
                if let Some(component) = factory(&ticket) {
                    let enabled = match &condition {
                        Some(c) => evaluate_condition(c, &*context.borrow()),
                        None => true,
                    };
                    $self.$target.push(ComponentEntry {
                        component: RefCell::new(component),
                        condition,
                        enabled: Cell::new(enabled),
                    });
                }
            }
        }
    }};
}

/// The concrete input-method engine.
pub struct Engine {
    context: ContextRc,
    schema: Schema,
    processors: Vec<ComponentEntry<dyn Processor>>,
    segmentors: Vec<ComponentEntry<dyn Segmentor>>,
    translators: Vec<ComponentEntry<dyn Translator>>,
    filters: Vec<ComponentEntry<dyn Filter>>,
    formatters: RefCell<Vec<Box<dyn Formatter>>>,
    post_processors: RefCell<Vec<Box<dyn Processor>>>,
    sink: RefCell<Vec<Box<dyn FnMut(String)>>>,
    message_sink: RefCell<Vec<Box<dyn FnMut(String, String)>>>,
}

impl Engine {
    pub fn new(schema: Schema) -> EngineRef {
        let context = Rc::new(RefCell::new(Context::new()));
        let mut engine = Engine {
            context: context.clone(),
            schema,
            processors: Vec::new(),
            segmentors: Vec::new(),
            translators: Vec::new(),
            filters: Vec::new(),
            formatters: RefCell::new(Vec::new()),
            post_processors: RefCell::new(Vec::new()),
            sink: RefCell::new(Vec::new()),
            message_sink: RefCell::new(Vec::new()),
        };
        // Initialize components and options before wiring notifiers so that
        // option updates during initialization do not re-enter the engine.
        engine.initialize_components();
        engine.initialize_options();
        let engine = Rc::new(engine);
        engine.wire_notifiers(engine.clone());
        engine
    }

    pub fn context(&self) -> &ContextRc {
        &self.context
    }

    pub fn schema(&self) -> &Schema {
        &self.schema
    }

    pub fn connect_sink(&self, f: Box<dyn FnMut(String)>) {
        self.sink.borrow_mut().push(f);
    }

    pub fn connect_message_sink(&self, f: Box<dyn FnMut(String, String)>) {
        self.message_sink.borrow_mut().push(f);
    }

    fn wire_notifiers(&self, engine: EngineRef) {
        let ctx = self.context.clone();
        let e = engine.clone();
        ctx.borrow_mut()
            .on_update(Box::new(move |c: &mut Context| e.compose(c)));
        let ctx = self.context.clone();
        let e = engine.clone();
        ctx.borrow_mut()
            .on_commit(Box::new(move |c: &mut Context| e.on_commit(c)));
        let ctx = self.context.clone();
        let e = engine.clone();
        ctx.borrow_mut()
            .on_select(Box::new(move |c: &mut Context| e.on_select(c)));
        let ctx = self.context.clone();
        let e = engine.clone();
        ctx.borrow_mut()
            .on_option_update(Box::new(move |c: &mut Context, o: &str| e.on_option_update(c, o)));
        let ctx = self.context.clone();
        ctx.borrow_mut()
            .on_property_update(Box::new(move |c: &mut Context, p: &str| {
                engine.on_property_update(c, p)
            }));
    }

    // --- entry points ---

    pub fn process_key(&self, key_event: &KeyEvent) -> bool {
        self.process_key_inner(key_event, false)
    }

    pub fn process_synthetic_key(&self, key_event: &KeyEvent) -> bool {
        self.process_key_inner(key_event, true)
    }

    fn process_key_inner(&self, key_event: &KeyEvent, synthetic: bool) -> bool {
        for entry in &self.processors {
            if !entry.enabled.get() {
                continue;
            }
            match entry.component.borrow_mut().process_key_event(key_event) {
                ProcessResult::Rejected => break,
                ProcessResult::Accepted => return true,
                ProcessResult::Noop => {}
            }
        }
        if !synthetic {
            self.context
                .borrow_mut()
                .commit_history_mut()
                .push_key_event(key_event);
        }
        for processor in self.post_processors.borrow_mut().iter_mut() {
            match processor.process_key_event(key_event) {
                ProcessResult::Rejected => break,
                ProcessResult::Accepted => return true,
                ProcessResult::Noop => {}
            }
        }
        self.context.borrow_mut().notify_unhandled_key(key_event);
        false
    }

    pub fn commit_text(&self, text: String) {
        self.context
            .borrow_mut()
            .commit_history_mut()
            .push(CommitRecord::new("raw", &text));
        let mut text = text;
        self.format_text(&mut text);
        for sink in self.sink.borrow_mut().iter_mut() {
            sink(text.clone());
        }
    }

    // --- composition pipeline ---

    fn compose(&self, ctx: &mut Context) {
        let input = ctx.input().to_string();
        let caret_pos = ctx.caret_pos();
        let active_input = substr(&input, 0, caret_pos).to_string();
        let comp = ctx.composition_mut();
        comp.reset(&active_input);
        let confirmed = comp.get_confirmed_position();
        if caret_pos < input.len() && caret_pos == confirmed {
            comp.reset(&input);
        }
        let segments = comp.as_segmentation_mut();
        self.calculate_segmentation(segments, caret_pos);
        self.translate_segments(segments);
    }

    fn calculate_segmentation(&self, segments: &mut Segmentation, caret_pos: usize) {
        while !segments.has_finished_segmentation() {
            let start_pos = segments.get_current_start_position();
            for entry in &self.segmentors {
                if !entry.enabled.get() {
                    continue;
                }
                if !entry.component.borrow_mut().proceed(segments) {
                    break;
                }
            }
            if start_pos == segments.get_current_end_position() {
                break;
            }
            if start_pos >= caret_pos {
                break;
            }
            if !segments.has_finished_segmentation() {
                segments.forward();
            }
        }
        if !segments.is_empty() && !segments.last().unwrap().has_tag("placeholder") {
            segments.trim();
        }
        if !segments.is_empty() && segments.last().unwrap().status >= SegmentStatus::Selected {
            segments.forward();
        }
    }

    fn translate_segments(&self, segments: &mut Segmentation) {
        let input = segments.input().to_string();
        for segment in segments.iter_mut() {
            if segment.status >= SegmentStatus::Guess {
                continue;
            }
            let len = segment.end - segment.start;
            let seg_input = substr(&input, segment.start, segment.start + len).to_string();
            let mut menu = Menu::new();
            for entry in &self.translators {
                if !entry.enabled.get() {
                    continue;
                }
                if let Some(translation) = entry.component.borrow_mut().query(&seg_input, segment) {
                    if !translation.borrow().exhausted() {
                        menu.add_translation(translation);
                    }
                }
            }
            for entry in &self.filters {
                if entry.enabled.get() && entry.component.borrow().applies_to_segment(segment) {
                    menu.add_filter(entry.component.borrow_mut().as_mut());
                }
            }
            segment.status = SegmentStatus::Guess;
            segment.menu = Some(Rc::new(RefCell::new(menu)));
            segment.selected_index = 0;
        }
    }

    fn format_text(&self, text: &mut String) {
        for formatter in self.formatters.borrow_mut().iter_mut() {
            formatter.format(text);
        }
    }

    // --- context notifications ---

    fn on_commit(&self, ctx: &mut Context) {
        ctx.record_commit();
        let mut text = ctx.get_commit_text();
        self.format_text(&mut text);
        for sink in self.sink.borrow_mut().iter_mut() {
            sink(text.clone());
        }
    }

    fn on_select(&self, ctx: &mut Context) {
        enum Action {
            Commit,
            Forward,
            Advance { reached_caret: bool },
        }
        let input_len = ctx.input().len();
        let caret_pos = ctx.caret_pos();
        let auto_commit = ctx.get_option("_auto_commit");
        let action = {
            let seg = ctx.composition_mut().last_mut().unwrap();
            seg.close();
            if seg.end == input_len {
                seg.status = SegmentStatus::Confirmed;
                if auto_commit {
                    Action::Commit
                } else {
                    Action::Forward
                }
            } else {
                Action::Advance {
                    reached_caret: seg.end >= caret_pos,
                }
            }
        };
        match action {
            Action::Commit => {
                ctx.commit();
            }
            Action::Forward => {
                ctx.composition_mut().forward();
            }
            Action::Advance { reached_caret } => {
                ctx.composition_mut().forward();
                if reached_caret {
                    let len = ctx.input().len();
                    ctx.set_caret_pos(len);
                } else {
                    self.compose(ctx);
                }
            }
        }
    }

    fn on_option_update(&self, ctx: &mut Context, option: &str) {
        update_conditions(&self.processors, ctx);
        update_conditions(&self.segmentors, ctx);
        update_conditions(&self.translators, ctx);
        update_conditions(&self.filters, ctx);
        if ctx.is_composing() {
            ctx.refresh_non_confirmed_composition();
        }
        let option_is_on = ctx.get_option(option);
        let msg = if option_is_on {
            option.to_string()
        } else {
            format!("!{}", option)
        };
        for sink in self.message_sink.borrow_mut().iter_mut() {
            sink("option".to_string(), msg.clone());
        }
    }

    fn on_property_update(&self, ctx: &mut Context, property: &str) {
        let value = ctx.get_property(property);
        let msg = format!("{}={}", property, value);
        for sink in self.message_sink.borrow_mut().iter_mut() {
            sink("property".to_string(), msg.clone());
        }
    }

    // --- initialization (called once with `&mut self` before sharing) ---

    fn initialize_components(&mut self) {
        self.processors.clear();
        self.segmentors.clear();
        self.translators.clear();
        self.filters.clear();
        self.formatters.borrow_mut().clear();
        self.post_processors.borrow_mut().clear();

        if self.schema.config().get("engine").is_none() {
            return;
        }
        create_components!(self, "engine/processors", "processor", require_processor, processors);
        create_components!(self, "engine/segmentors", "segmentor", require_segmentor, segmentors);
        create_components!(self, "engine/translators", "translator", require_translator, translators);
        create_components!(self, "engine/filters", "filter", require_filter, filters);

        let schema_config = self.schema.config().clone();
        if let Some(factory) = crate::component::require_formatter("shape_formatter") {
            let ticket = Ticket {
                context: self.context.clone(),
                schema_config: schema_config.clone(),
                name_space: "formatter".to_string(),
                klass: "shape_formatter".to_string(),
            };
            if let Some(f) = factory(&ticket) {
                self.formatters.borrow_mut().push(f);
            }
        }
        if let Some(factory) = crate::component::require_processor("shape_processor") {
            let ticket = Ticket {
                context: self.context.clone(),
                schema_config,
                name_space: "processor".to_string(),
                klass: "shape_processor".to_string(),
            };
            if let Some(p) = factory(&ticket) {
                self.post_processors.borrow_mut().push(p);
            }
        }
    }

    fn initialize_options(&mut self) {
        let config = self.schema.config().clone();
        let ctx = self.context.clone();
        let switches = Switches::new(&config);
        switches.find_option(|option| {
            if option.reset_value >= 0 {
                if option.type_ == SwitchType::ToggleOption {
                    ctx.borrow_mut()
                        .set_option(&option.option_name, option.reset_value != 0);
                } else if option.type_ == SwitchType::RadioGroup {
                    ctx.borrow_mut().set_option(
                        &option.option_name,
                        option.option_index as i32 == option.reset_value,
                    );
                }
            }
            FindResult::Continue
        });
        let ctx = self.context.borrow();
        update_conditions(&self.processors, &ctx);
        update_conditions(&self.segmentors, &ctx);
        update_conditions(&self.translators, &ctx);
        update_conditions(&self.filters, &ctx);
    }
}

impl std::fmt::Debug for Engine {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Engine")
            .field("schema", &self.schema.schema_id())
            .field("processors", &self.processors.len())
            .field("segmentors", &self.segmentors.len())
            .field("translators", &self.translators.len())
            .field("filters", &self.filters.len())
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::candidate::{CandidateArc, SimpleCandidate};
    use crate::component::{register_processor, register_segmentor, register_translator};
    use crate::config::{Config, ConfigItem};
    use crate::translation::{new_translation, FifoTranslation, TranslationRc};
    use std::collections::BTreeMap;

    struct TrivialProcessor;
    impl Processor for TrivialProcessor {
        fn process_key_event(&mut self, key_event: &KeyEvent) -> ProcessResult {
            if key_event.modifier() == 0 && (0x20..=0x7e).contains(&key_event.keycode()) {
                ProcessResult::Accepted
            } else {
                ProcessResult::Noop
            }
        }
    }

    struct TrivialSegmentor;
    impl Segmentor for TrivialSegmentor {
        fn proceed(&mut self, segmentation: &mut Segmentation) -> bool {
            let start = segmentation.get_current_start_position();
            let input_len = segmentation.input().len();
            if start < input_len {
                segmentation
                    .add_segment(crate::segmentation::Segment::with_range(start, input_len));
            }
            true
        }
    }

    struct TrivialTranslator;
    impl Translator for TrivialTranslator {
        fn query(
            &mut self,
            input: &str,
            _segment: &crate::segmentation::Segment,
        ) -> Option<TranslationRc> {
            let mut t = FifoTranslation::new();
            let cand: CandidateArc = std::sync::Arc::new(SimpleCandidate::new(
                "trivial",
                0,
                input.len(),
                &format!("[{}]", input),
                "",
                "",
            ));
            t.append(cand);
            Some(new_translation(t))
        }
    }

    fn test_schema() -> Schema {
        let mut engine = BTreeMap::new();
        engine.insert(
            "processors".to_string(),
            ConfigItem::List(vec![ConfigItem::Scalar("trivial_processor".to_string())]),
        );
        engine.insert(
            "segmentors".to_string(),
            ConfigItem::List(vec![ConfigItem::Scalar("trivial_segmentor".to_string())]),
        );
        engine.insert(
            "translators".to_string(),
            ConfigItem::List(vec![ConfigItem::Scalar("trivial_translator".to_string())]),
        );
        let mut root = BTreeMap::new();
        root.insert("engine".to_string(), ConfigItem::Map(engine));
        root.insert(
            "menu".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "page_size".to_string(),
                ConfigItem::Scalar("5".to_string()),
            )])),
        );
        let config = Rc::new(Config::new(ConfigItem::Map(root)));
        Schema::new("test", config)
    }

    #[test]
    fn engine_pipeline_produces_candidate() {
        register_processor("trivial_processor", |_ticket| Some(Box::new(TrivialProcessor)));
        register_segmentor("trivial_segmentor", |_ticket| Some(Box::new(TrivialSegmentor)));
        register_translator("trivial_translator", |_ticket| Some(Box::new(TrivialTranslator)));

        let engine = Engine::new(test_schema());
        engine
            .context()
            .borrow_mut()
            .push_input_str("abc");

        let cand = engine.context().borrow().get_selected_candidate();
        assert!(cand.is_some());
        assert_eq!(cand.unwrap().text(), "[abc]");
    }

    #[test]
    fn condition_toggles_component() {
        register_processor("cond_processor", |_ticket| Some(Box::new(TrivialProcessor)));
        let mut engine_map = BTreeMap::new();
        engine_map.insert(
            "processors".to_string(),
            ConfigItem::List(vec![ConfigItem::Map(BTreeMap::from([
                ("name".to_string(), ConfigItem::Scalar("cond_processor".to_string())),
                ("when".to_string(), ConfigItem::Scalar("ascii_mode".to_string())),
            ]))]),
        );
        let root = BTreeMap::from([("engine".to_string(), ConfigItem::Map(engine_map))]);
        let config = Rc::new(Config::new(ConfigItem::Map(root)));
        let engine = Engine::new(Schema::new("test", config));
        assert!(!engine.processors[0].enabled.get());
        engine
            .context()
            .borrow_mut()
            .set_option("ascii_mode", true);
        assert!(engine.processors[0].enabled.get());
    }
}
