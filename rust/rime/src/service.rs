// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/service.h and service.cc — session management and the
// Rust-native Service API.

use std::cell::RefCell;
use std::collections::BTreeMap;
use std::rc::Rc;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::engine::Engine;
use crate::key_event::KeyEvent;
use crate::schema::Schema;

pub type SessionId = usize;
pub const K_INVALID_SESSION_ID: SessionId = 0;

const K_LIFE_SPAN_SECS: u64 = 5 * 60;

fn now_secs() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_secs())
        .unwrap_or(0)
}

/// An input session: an engine plus accumulated commit text.
pub struct Session {
    engine: Rc<Engine>,
    commit_text: Rc<RefCell<String>>,
    last_active_time: RefCell<u64>,
}

impl Session {
    pub fn new(schema: Schema) -> Rc<Session> {
        let engine = Engine::new(schema);
        let commit_text = Rc::new(RefCell::new(String::new()));
        let ct = Rc::clone(&commit_text);
        engine.connect_sink(Box::new(move |text: String| {
            ct.borrow_mut().push_str(&text);
        }));
        Rc::new(Session {
            engine,
            commit_text,
            last_active_time: RefCell::new(now_secs()),
        })
    }

    pub fn process_key(&self, key_event: &KeyEvent) -> bool {
        self.engine.process_key(key_event)
    }

    pub fn activate(&self) {
        *self.last_active_time.borrow_mut() = now_secs();
    }

    pub fn reset_commit_text(&self) {
        self.commit_text.borrow_mut().clear();
    }

    pub fn commit_composition(&self) -> bool {
        self.engine.context().borrow_mut().commit();
        !self.commit_text.borrow().is_empty()
    }

    pub fn clear_composition(&self) {
        self.engine.context().borrow_mut().abort_composition();
    }

    pub fn context(&self) -> Rc<RefCell<crate::context::Context>> {
        self.engine.context().clone()
    }

    pub fn schema(&self) -> &Schema {
        self.engine.schema()
    }

    pub fn last_active_time(&self) -> u64 {
        *self.last_active_time.borrow()
    }

    pub fn commit_text(&self) -> String {
        self.commit_text.borrow().clone()
    }
}

/// Manages input sessions (mirrors `Service`).
pub struct Service {
    sessions: BTreeMap<SessionId, Rc<Session>>,
    next_id: SessionId,
    notification_handler: Option<Box<dyn Fn(SessionId, &str, &str)>>,
}

impl Default for Service {
    fn default() -> Self {
        Self::new()
    }
}

impl Service {
    pub fn new() -> Self {
        Self {
            sessions: BTreeMap::new(),
            next_id: 1,
            notification_handler: None,
        }
    }

    pub fn create_session(&mut self, schema: Schema) -> SessionId {
        let session = Session::new(schema);
        let id = self.next_id;
        self.next_id += 1;
        self.sessions.insert(id, session);
        id
    }

    pub fn get_session(&self, id: SessionId) -> Option<Rc<Session>> {
        self.sessions.get(&id).cloned()
    }

    pub fn destroy_session(&mut self, id: SessionId) -> bool {
        self.sessions.remove(&id).is_some()
    }

    pub fn cleanup_all_sessions(&mut self) {
        self.sessions.clear();
    }

    pub fn cleanup_stale_sessions(&mut self) {
        let now = now_secs();
        self.sessions
            .retain(|_, s| s.last_active_time() + K_LIFE_SPAN_SECS >= now);
    }

    pub fn set_notification_handler(&mut self, handler: Option<Box<dyn Fn(SessionId, &str, &str)>>) {
        self.notification_handler = handler;
    }

    pub fn notify(&self, session_id: SessionId, message_type: &str, message_value: &str) {
        if let Some(handler) = &self.notification_handler {
            handler(session_id, message_type, message_value);
        }
    }
}

thread_local! {
    static SERVICE: RefCell<Service> = RefCell::new(Service::new());
}

/// Access the thread-local service (used by the C ABI).
pub fn with_service<R>(f: impl FnOnce(&mut Service) -> R) -> R {
    SERVICE.with(|s| f(&mut s.borrow_mut()))
}

/// Create a session in the thread-local service using the given schema.
pub fn create_session(schema: Schema) -> SessionId {
    with_service(|s| s.create_session(schema))
}

pub fn get_session(id: SessionId) -> Option<Rc<Session>> {
    SERVICE.with(|s| s.borrow().get_session(id))
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::config::{Config, ConfigItem};
    use crate::component::{register_segmentor, register_translator};
    use crate::dict::dictionary::{register_dictionary, Dictionary};
    use crate::dict::table::Table;
    use crate::gear::abc_segmentor::AbcSegmentor;
    use crate::gear::table_translator::TableTranslator;
    use crate::key_event::KeyEvent;
    use std::collections::BTreeMap;

    fn pinyin_schema() -> Schema {
        register_segmentor("abc_segmentor", |t| Some(Box::new(AbcSegmentor::new(t))));
        register_translator("table_translator", |t| Some(Box::new(TableTranslator::new(t))));
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
                ("dictionary".to_string(), ConfigItem::Scalar("pinyin".to_string())),
                ("delimiter".to_string(), ConfigItem::Scalar(" ".to_string())),
            ])),
        );
        Schema::new("pinyin", Rc::new(Config::new(ConfigItem::Map(root))))
    }

    #[test]
    fn session_processes_keys_and_commits() {
        let mut service = Service::new();
        let id = service.create_session(pinyin_schema());
        let session = service.get_session(id).unwrap();

        // Simulate typing via direct input insertion, then commit.
        session.context().borrow_mut().push_input_str("ni hao");
        assert_eq!(
            session.context().borrow().get_selected_candidate().unwrap().text(),
            "你好"
        );

        assert!(session.commit_composition());
        assert_eq!(session.commit_text(), "你好");

        assert!(service.destroy_session(id));
        assert!(service.get_session(id).is_none());
    }

    #[test]
    fn service_cleans_up_stale_sessions() {
        let mut service = Service::new();
        let id = service.create_session(pinyin_schema());
        // age the session out
        *service.sessions.get_mut(&id).unwrap().last_active_time.borrow_mut() = 0;
        service.cleanup_stale_sessions();
        assert!(service.get_session(id).is_none());
    }

    #[test]
    fn process_key_accepts_printable() {
        let mut service = Service::new();
        let id = service.create_session(pinyin_schema());
        let session = service.get_session(id).unwrap();
        // no processor accepts in this minimal schema, so the key falls through
        let key = KeyEvent::from_repr("a");
        assert!(!session.process_key(&key));
    }
}
