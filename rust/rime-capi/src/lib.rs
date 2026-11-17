// Copyright RIME Developers
// Distributed under the BSD License
//
// A C ABI for the Rust Rime engine, mirroring the core of rime_api.h.
// This is a reduced, self-contained surface (session management, key
// processing, commit/context/status output, options) sufficient for a minimal
// frontend; the full rime_api.h layout (config, deployment, schema list, ...)
// can be layered on later.

use std::cell::RefCell;
use std::collections::{BTreeMap, HashMap};
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};

use rime::candidate::CandidateArc;
use rime::config::{Config, ConfigItem};
use rime::schema::Schema;
use rime::service::{self, SessionId};

pub type Bool = i32;
pub const FALSE: Bool = 0;
pub const TRUE: Bool = 1;
pub type RimeSessionId = usize;

// --- C-compatible structs ---

#[repr(C)]
pub struct RimeTraits {
    pub data_size: i32,
    pub shared_data_dir: *const c_char,
    pub user_data_dir: *const c_char,
    pub distribution_name: *const c_char,
    pub distribution_code_name: *const c_char,
    pub distribution_version: *const c_char,
    pub app_name: *const c_char,
    pub modules: *const *const c_char,
    pub min_log_level: i32,
    pub log_dir: *const c_char,
    pub prebuilt_data_dir: *const c_char,
    pub staging_dir: *const c_char,
}

#[repr(C)]
pub struct RimeComposition {
    pub length: i32,
    pub cursor_pos: i32,
    pub sel_start: i32,
    pub sel_end: i32,
    pub preedit: *mut c_char,
}

#[repr(C)]
pub struct RimeCandidate {
    pub text: *mut c_char,
    pub comment: *mut c_char,
    pub reserved: *mut c_void,
}

#[repr(C)]
pub struct RimeMenu {
    pub page_size: i32,
    pub page_no: i32,
    pub is_last_page: Bool,
    pub highlighted_candidate_index: i32,
    pub num_candidates: i32,
    pub candidates: *mut RimeCandidate,
    pub select_keys: *mut c_char,
}

#[repr(C)]
pub struct RimeCommit {
    pub data_size: i32,
    pub text: *mut c_char,
}

#[repr(C)]
pub struct RimeContext {
    pub data_size: i32,
    pub composition: RimeComposition,
    pub menu: RimeMenu,
    pub commit_text_preview: *mut c_char,
    pub select_labels: *mut *mut c_char,
}

#[repr(C)]
pub struct RimeStatus {
    pub data_size: i32,
    pub schema_id: *mut c_char,
    pub schema_name: *mut c_char,
    pub is_disabled: Bool,
    pub is_composing: Bool,
    pub is_ascii_mode: Bool,
    pub is_full_shape: Bool,
    pub is_simplified: Bool,
    pub is_traditional: Bool,
    pub is_ascii_punct: Bool,
}

pub type RimeNotificationHandler = Option<
    extern "C" fn(context_object: *mut c_void, session_id: RimeSessionId, message_type: *const c_char, message_value: *const c_char),
>;

// --- tracked allocations (cleaned up by the matching free_* call) ---

type Cleanup = Box<dyn FnOnce()>;

thread_local! {
    static CLEANUPS: RefCell<Vec<Cleanup>> = RefCell::new(Vec::new());
    static DEFAULT_SCHEMA: RefCell<Option<Schema>> = RefCell::new(None);
    static SESSION_INPUTS: RefCell<Vec<(SessionId, String)>> = RefCell::new(Vec::new());
}

fn track(f: impl FnOnce() + 'static) {
    CLEANUPS.with(|c| c.borrow_mut().push(Box::new(f)));
}

/// Configure the schema used by subsequently created sessions (Rust setup hook).
pub fn set_default_schema(schema: Schema) {
    DEFAULT_SCHEMA.with(|s| *s.borrow_mut() = Some(schema));
}

fn take_default_schema() -> Option<Schema> {
    DEFAULT_SCHEMA.with(|s| s.borrow_mut().take())
}

fn into_cstring(s: String) -> *mut c_char {
    CString::new(s).unwrap_or_default().into_raw()
}

fn cstr_to_string(ptr: *const c_char) -> String {
    unsafe { CStr::from_ptr(ptr) }.to_string_lossy().into_owned()
}

fn copy_to_buffer(src: &str, dst: *mut c_char, buffer_size: usize) {
    if dst.is_null() || buffer_size == 0 {
        return;
    }
    let bytes = src.as_bytes();
    let n = bytes.len().min(buffer_size - 1);
    unsafe {
        std::ptr::copy_nonoverlapping(bytes.as_ptr(), dst as *mut u8, n);
        *dst.add(n) = 0;
    }
}

// --- C API implementation ---

#[no_mangle]
pub extern "C" fn RimeGetVersion() -> *const c_char {
    static VERSION: &[u8] = b"1.11.0-rust\0";
    VERSION.as_ptr() as *const c_char
}

#[no_mangle]
pub extern "C" fn RimeSetup(_traits: *mut RimeTraits) {}

#[no_mangle]
pub extern "C" fn RimeInitialize(_traits: *mut RimeTraits) {}

#[no_mangle]
pub extern "C" fn RimeFinalize() {
    RimeCleanupAllSessions();
}

#[no_mangle]
pub extern "C" fn RimeCreateSession() -> RimeSessionId {
    match take_default_schema() {
        Some(schema) => service::create_session(schema),
        None => 0,
    }
}

#[no_mangle]
pub extern "C" fn RimeFindSession(session_id: RimeSessionId) -> Bool {
    if service::get_session(session_id).is_some() {
        TRUE
    } else {
        FALSE
    }
}

#[no_mangle]
pub extern "C" fn RimeDestroySession(session_id: RimeSessionId) -> Bool {
    if service::with_service(|s| s.destroy_session(session_id)) {
        TRUE
    } else {
        FALSE
    }
}

#[no_mangle]
pub extern "C" fn RimeCleanupAllSessions() {
    service::with_service(|s| s.cleanup_all_sessions());
}

#[no_mangle]
pub extern "C" fn RimeProcessKey(session_id: RimeSessionId, keycode: i32, mask: i32) -> Bool {
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    let key = rime::key_event::KeyEvent::with_keycode(keycode, mask);
    if session.process_key(&key) {
        TRUE
    } else {
        FALSE
    }
}

#[no_mangle]
pub extern "C" fn RimeCommitComposition(session_id: RimeSessionId) -> Bool {
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    if session.commit_composition() {
        TRUE
    } else {
        FALSE
    }
}

#[no_mangle]
pub extern "C" fn RimeClearComposition(session_id: RimeSessionId) {
    if let Some(session) = service::get_session(session_id) {
        session.clear_composition();
    }
}

#[no_mangle]
pub extern "C" fn RimeGetCommit(session_id: RimeSessionId, commit: *mut RimeCommit) -> Bool {
    if commit.is_null() {
        return FALSE;
    }
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    let text = session.commit_text();
    if text.is_empty() {
        return FALSE;
    }
    let ptr = into_cstring(text);
    unsafe {
        (*commit).data_size = std::mem::size_of::<RimeCommit>() as i32 - 4;
        (*commit).text = ptr;
    }
    session.reset_commit_text();
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeFreeCommit(commit: *mut RimeCommit) -> Bool {
    if commit.is_null() {
        return FALSE;
    }
    unsafe {
        if !(*commit).text.is_null() {
            drop(CString::from_raw((*commit).text));
            (*commit).text = std::ptr::null_mut();
        }
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeGetContext(session_id: RimeSessionId, context: *mut RimeContext) -> Bool {
    if context.is_null() {
        return FALSE;
    }
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    let ctx = session.context();
    let borrowed = ctx.borrow();
    let preedit = borrowed.get_preedit();
    let candidate = borrowed.get_selected_candidate();

    // preedit string
    let preedit_ptr = into_cstring(preedit.text.clone());
    track(move || drop(unsafe { CString::from_raw(preedit_ptr) }));

    // candidate list
    let cand_ptr = match &candidate {
        Some(c) => alloc_candidate(c),
        None => std::ptr::null_mut(),
    };

    unsafe {
        let c = &mut *context;
        c.data_size = std::mem::size_of::<RimeContext>() as i32 - 4;
        c.composition.length = borrowed.input().len() as i32;
        c.composition.cursor_pos = preedit.caret_pos as i32;
        c.composition.sel_start = preedit.sel_start as i32;
        c.composition.sel_end = preedit.sel_end as i32;
        c.composition.preedit = preedit_ptr;
        c.menu.page_size = 5;
        c.menu.page_no = 0;
        c.menu.is_last_page = TRUE;
        c.menu.highlighted_candidate_index = 0;
        c.menu.num_candidates = if candidate.is_some() { 1 } else { 0 };
        c.menu.candidates = cand_ptr;
        c.menu.select_keys = std::ptr::null_mut();
        c.commit_text_preview = std::ptr::null_mut();
        c.select_labels = std::ptr::null_mut();
    }
    TRUE
}

fn alloc_candidate(cand: &CandidateArc) -> *mut RimeCandidate {
    let text_ptr = into_cstring(cand.text().to_string());
    let comment_ptr = if cand.comment().is_empty() {
        std::ptr::null_mut()
    } else {
        into_cstring(cand.comment().to_string())
    };
    let boxed = Box::new(RimeCandidate {
        text: text_ptr,
        comment: comment_ptr,
        reserved: std::ptr::null_mut(),
    });
    let ptr = Box::into_raw(boxed);
    track(move || {
        unsafe {
            drop(CString::from_raw(text_ptr));
            if !comment_ptr.is_null() {
                drop(CString::from_raw(comment_ptr));
            }
            drop(Box::from_raw(ptr));
        }
    });
    ptr
}

#[no_mangle]
pub extern "C" fn RimeFreeContext(_context: *mut RimeContext) -> Bool {
    CLEANUPS.with(|c| {
        let mut cleanups = c.borrow_mut();
        for f in cleanups.drain(..) {
            f();
        }
    });
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeGetStatus(session_id: RimeSessionId, status: *mut RimeStatus) -> Bool {
    if status.is_null() {
        return FALSE;
    }
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    let schema_id = session.schema().schema_id().to_string();
    let schema_name = session.schema().schema_name().to_string();
    let ctx = session.context();
    let borrowed = ctx.borrow();
    let is_composing = borrowed.is_composing();
    let is_ascii_mode = borrowed.get_option("ascii_mode");
    unsafe {
        let s = &mut *status;
        s.data_size = std::mem::size_of::<RimeStatus>() as i32 - 4;
        s.schema_id = into_cstring(schema_id);
        s.schema_name = into_cstring(schema_name);
        s.is_disabled = FALSE;
        s.is_composing = if is_composing { TRUE } else { FALSE };
        s.is_ascii_mode = if is_ascii_mode { TRUE } else { FALSE };
        s.is_full_shape = FALSE;
        s.is_simplified = FALSE;
        s.is_traditional = FALSE;
        s.is_ascii_punct = FALSE;
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeFreeStatus(status: *mut RimeStatus) -> Bool {
    if status.is_null() {
        return FALSE;
    }
    unsafe {
        if !(*status).schema_id.is_null() {
            drop(CString::from_raw((*status).schema_id));
        }
        if !(*status).schema_name.is_null() {
            drop(CString::from_raw((*status).schema_name));
        }
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeSetOption(session_id: RimeSessionId, option: *const c_char, value: Bool) {
    if option.is_null() {
        return;
    }
    let Some(session) = service::get_session(session_id) else {
        return;
    };
    let name = unsafe { CStr::from_ptr(option) }.to_string_lossy().into_owned();
    session
        .context()
        .borrow_mut()
        .set_option(&name, value != FALSE);
}

#[no_mangle]
pub extern "C" fn RimeGetOption(session_id: RimeSessionId, option: *const c_char) -> Bool {
    if option.is_null() {
        return FALSE;
    }
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    let name = unsafe { CStr::from_ptr(option) }.to_string_lossy().into_owned();
    if session.context().borrow().get_option(&name) {
        TRUE
    } else {
        FALSE
    }
}

#[no_mangle]
pub extern "C" fn RimeGetInput(session_id: RimeSessionId) -> *const c_char {
    let Some(session) = service::get_session(session_id) else {
        return std::ptr::null();
    };
    let input = session.context().borrow().input().to_string();
    SESSION_INPUTS.with(|store| {
        let mut store = store.borrow_mut();
        if let Some((_, s)) = store.iter_mut().find(|(id, _)| *id == session_id) {
            *s = input;
            s.as_ptr() as *const c_char
        } else {
            store.push((session_id, input));
            store.last().unwrap().1.as_ptr() as *const c_char
        }
    })
}

#[no_mangle]
pub extern "C" fn RimeGetCaretPos(session_id: RimeSessionId) -> usize {
    service::get_session(session_id)
        .map(|s| s.context().borrow().caret_pos())
        .unwrap_or(0)
}

#[no_mangle]
pub extern "C" fn RimeSelectCandidate(session_id: RimeSessionId, index: usize) -> Bool {
    let Some(session) = service::get_session(session_id) else {
        return FALSE;
    };
    if session.context().borrow_mut().select(index) {
        TRUE
    } else {
        FALSE
    }
}

// --- configuration ---

#[repr(C)]
pub struct RimeConfig {
    pub ptr: *mut c_void,
}

thread_local! {
    static CONFIGS: RefCell<HashMap<String, Config>> = RefCell::new(HashMap::new());
}

/// Register a config under a name (Rust setup hook; used by RimeConfigOpen).
pub fn register_config(name: &str, config: Config) {
    CONFIGS.with(|c| c.borrow_mut().insert(name.to_string(), config));
}

fn config_ptr(config: *mut RimeConfig) -> *mut Config {
    if config.is_null() {
        return std::ptr::null_mut();
    }
    unsafe { (*config).ptr as *mut Config }
}

#[no_mangle]
pub extern "C" fn RimeConfigOpen(config_id: *const c_char, config: *mut RimeConfig) -> Bool {
    if config.is_null() {
        return FALSE;
    }
    let name = if config_id.is_null() {
        String::new()
    } else {
        cstr_to_string(config_id)
    };
    let cfg = CONFIGS
        .with(|c| c.borrow().get(&name).cloned())
        .unwrap_or_else(|| Config::new(ConfigItem::Map(BTreeMap::new())));
    unsafe {
        (*config).ptr = Box::into_raw(Box::new(cfg)) as *mut c_void;
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeSchemaOpen(schema_id: *const c_char, config: *mut RimeConfig) -> Bool {
    RimeConfigOpen(schema_id, config)
}

#[no_mangle]
pub extern "C" fn RimeConfigClose(config: *mut RimeConfig) -> Bool {
    if config.is_null() {
        return FALSE;
    }
    unsafe {
        if !(*config).ptr.is_null() {
            drop(Box::from_raw((*config).ptr as *mut Config));
            (*config).ptr = std::ptr::null_mut();
        }
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigInit(config: *mut RimeConfig) -> Bool {
    if config.is_null() {
        return FALSE;
    }
    unsafe {
        let cfg = Config::new(ConfigItem::Map(BTreeMap::new()));
        (*config).ptr = Box::into_raw(Box::new(cfg)) as *mut c_void;
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigLoadString(config: *mut RimeConfig, yaml: *const c_char) -> Bool {
    if config.is_null() || yaml.is_null() {
        return FALSE;
    }
    let cfg = Config::from_yaml(&cstr_to_string(yaml));
    unsafe {
        if !(*config).ptr.is_null() {
            drop(Box::from_raw((*config).ptr as *mut Config));
        }
        (*config).ptr = Box::into_raw(Box::new(cfg)) as *mut c_void;
    }
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigGetString(
    config: *mut RimeConfig,
    key: *const c_char,
    value: *mut c_char,
    buffer_size: usize,
) -> Bool {
    if key.is_null() || value.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    match unsafe { &*ptr }.get_string(&key) {
        Some(s) => {
            copy_to_buffer(s, value, buffer_size);
            TRUE
        }
        None => FALSE,
    }
}

#[no_mangle]
pub extern "C" fn RimeConfigGetCString(config: *mut RimeConfig, key: *const c_char) -> *const c_char {
    if key.is_null() {
        return std::ptr::null();
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return std::ptr::null();
    }
    match unsafe { &*ptr }.get_string(&key) {
        Some(s) => CString::new(s).unwrap_or_default().into_raw(),
        None => std::ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn RimeConfigGetBool(config: *mut RimeConfig, key: *const c_char, value: *mut Bool) -> Bool {
    if key.is_null() || value.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    match unsafe { &*ptr }.get_bool(&key) {
        Some(b) => {
            unsafe { *value = if b { TRUE } else { FALSE } };
            TRUE
        }
        None => FALSE,
    }
}

#[no_mangle]
pub extern "C" fn RimeConfigGetInt(config: *mut RimeConfig, key: *const c_char, value: *mut i32) -> Bool {
    if key.is_null() || value.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    match unsafe { &*ptr }.get_int(&key) {
        Some(i) => {
            unsafe { *value = i };
            TRUE
        }
        None => FALSE,
    }
}

#[no_mangle]
pub extern "C" fn RimeConfigGetDouble(config: *mut RimeConfig, key: *const c_char, value: *mut f64) -> Bool {
    if key.is_null() || value.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    match unsafe { &*ptr }.get_double(&key) {
        Some(d) => {
            unsafe { *value = d };
            TRUE
        }
        None => FALSE,
    }
}

#[no_mangle]
pub extern "C" fn RimeConfigSetString(config: *mut RimeConfig, key: *const c_char, value: *const c_char) -> Bool {
    if key.is_null() || value.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let value = cstr_to_string(value);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    unsafe { &mut *ptr }.set_string(&key, &value);
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigSetBool(config: *mut RimeConfig, key: *const c_char, value: Bool) -> Bool {
    if key.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    unsafe { &mut *ptr }.set_bool(&key, value != FALSE);
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigSetInt(config: *mut RimeConfig, key: *const c_char, value: i32) -> Bool {
    if key.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    unsafe { &mut *ptr }.set_int(&key, value);
    TRUE
}

#[no_mangle]
pub extern "C" fn RimeConfigSetDouble(config: *mut RimeConfig, key: *const c_char, value: f64) -> Bool {
    if key.is_null() {
        return FALSE;
    }
    let key = cstr_to_string(key);
    let ptr = config_ptr(config);
    if ptr.is_null() {
        return FALSE;
    }
    unsafe { &mut *ptr }.set_double(&key, value);
    TRUE
}

// --- rime_get_api ---

#[repr(C)]
pub struct RimeApi {
    pub data_size: i32,
    pub setup: Option<extern "C" fn(*mut RimeTraits)>,
    pub initialize: Option<extern "C" fn(*mut RimeTraits)>,
    pub finalize: Option<extern "C" fn()>,
    pub create_session: Option<extern "C" fn() -> RimeSessionId>,
    pub find_session: Option<extern "C" fn(RimeSessionId) -> Bool>,
    pub destroy_session: Option<extern "C" fn(RimeSessionId) -> Bool>,
    pub cleanup_all_sessions: Option<extern "C" fn()>,
    pub process_key: Option<extern "C" fn(RimeSessionId, i32, i32) -> Bool>,
    pub commit_composition: Option<extern "C" fn(RimeSessionId) -> Bool>,
    pub clear_composition: Option<extern "C" fn(RimeSessionId)>,
    pub get_commit: Option<extern "C" fn(RimeSessionId, *mut RimeCommit) -> Bool>,
    pub free_commit: Option<extern "C" fn(*mut RimeCommit) -> Bool>,
    pub get_context: Option<extern "C" fn(RimeSessionId, *mut RimeContext) -> Bool>,
    pub free_context: Option<extern "C" fn(*mut RimeContext) -> Bool>,
    pub get_status: Option<extern "C" fn(RimeSessionId, *mut RimeStatus) -> Bool>,
    pub free_status: Option<extern "C" fn(*mut RimeStatus) -> Bool>,
    pub set_option: Option<extern "C" fn(RimeSessionId, *const c_char, Bool)>,
    pub get_option: Option<extern "C" fn(RimeSessionId, *const c_char) -> Bool>,
    pub get_version: Option<extern "C" fn() -> *const c_char>,
    pub get_input: Option<extern "C" fn(RimeSessionId) -> *const c_char>,
    pub get_caret_pos: Option<extern "C" fn(RimeSessionId) -> usize>,
    pub select_candidate: Option<extern "C" fn(RimeSessionId, usize) -> Bool>,
    // configuration
    pub schema_open: Option<extern "C" fn(*const c_char, *mut RimeConfig) -> Bool>,
    pub config_open: Option<extern "C" fn(*const c_char, *mut RimeConfig) -> Bool>,
    pub config_close: Option<extern "C" fn(*mut RimeConfig) -> Bool>,
    pub config_get_bool: Option<extern "C" fn(*mut RimeConfig, *const c_char, *mut Bool) -> Bool>,
    pub config_get_int: Option<extern "C" fn(*mut RimeConfig, *const c_char, *mut i32) -> Bool>,
    pub config_get_double: Option<extern "C" fn(*mut RimeConfig, *const c_char, *mut f64) -> Bool>,
    pub config_get_string:
        Option<extern "C" fn(*mut RimeConfig, *const c_char, *mut c_char, usize) -> Bool>,
    pub config_get_cstring: Option<extern "C" fn(*mut RimeConfig, *const c_char) -> *const c_char>,
    pub config_set_bool: Option<extern "C" fn(*mut RimeConfig, *const c_char, Bool) -> Bool>,
    pub config_set_int: Option<extern "C" fn(*mut RimeConfig, *const c_char, i32) -> Bool>,
    pub config_set_double: Option<extern "C" fn(*mut RimeConfig, *const c_char, f64) -> Bool>,
    pub config_set_string: Option<extern "C" fn(*mut RimeConfig, *const c_char, *const c_char) -> Bool>,
    pub config_init: Option<extern "C" fn(*mut RimeConfig) -> Bool>,
    pub config_load_string: Option<extern "C" fn(*mut RimeConfig, *const c_char) -> Bool>,
}

fn make_api() -> RimeApi {
    RimeApi {
        data_size: std::mem::size_of::<RimeApi>() as i32 - 4,
        setup: Some(RimeSetup),
        initialize: Some(RimeInitialize),
        finalize: Some(RimeFinalize),
        create_session: Some(RimeCreateSession),
        find_session: Some(RimeFindSession),
        destroy_session: Some(RimeDestroySession),
        cleanup_all_sessions: Some(RimeCleanupAllSessions),
        process_key: Some(RimeProcessKey),
        commit_composition: Some(RimeCommitComposition),
        clear_composition: Some(RimeClearComposition),
        get_commit: Some(RimeGetCommit),
        free_commit: Some(RimeFreeCommit),
        get_context: Some(RimeGetContext),
        free_context: Some(RimeFreeContext),
        get_status: Some(RimeGetStatus),
        free_status: Some(RimeFreeStatus),
        set_option: Some(RimeSetOption),
        get_option: Some(RimeGetOption),
        get_version: Some(RimeGetVersion),
        get_input: Some(RimeGetInput),
        get_caret_pos: Some(RimeGetCaretPos),
        select_candidate: Some(RimeSelectCandidate),
        schema_open: Some(RimeSchemaOpen),
        config_open: Some(RimeConfigOpen),
        config_close: Some(RimeConfigClose),
        config_get_bool: Some(RimeConfigGetBool),
        config_get_int: Some(RimeConfigGetInt),
        config_get_double: Some(RimeConfigGetDouble),
        config_get_string: Some(RimeConfigGetString),
        config_get_cstring: Some(RimeConfigGetCString),
        config_set_bool: Some(RimeConfigSetBool),
        config_set_int: Some(RimeConfigSetInt),
        config_set_double: Some(RimeConfigSetDouble),
        config_set_string: Some(RimeConfigSetString),
        config_init: Some(RimeConfigInit),
        config_load_string: Some(RimeConfigLoadString),
    }
}

#[no_mangle]
pub extern "C" fn rime_get_api() -> *const RimeApi {
    static API: std::sync::OnceLock<RimeApi> = std::sync::OnceLock::new();
    API.get_or_init(make_api)
}

#[cfg(test)]
mod tests {
    use super::*;
    use rime::component::{register_segmentor, register_translator};
    use rime::config::{Config, ConfigItem};
    use rime::dict::dictionary::{register_dictionary, Dictionary};
    use rime::dict::table::Table;
    use rime::gear::abc_segmentor::AbcSegmentor;
    use rime::gear::table_translator::TableTranslator;
    use std::collections::BTreeMap;
    use std::rc::Rc;

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
    fn capi_roundtrip() {
        set_default_schema(pinyin_schema());
        let id = RimeCreateSession();
        assert_ne!(id, 0);

        // insert input directly through the Rust session
        let session = rime::service::get_session(id).unwrap();
        session.context().borrow_mut().push_input_str("ni hao");

        // get context
        let mut ctx = RimeContext {
            data_size: 0,
            composition: RimeComposition { length: 0, cursor_pos: 0, sel_start: 0, sel_end: 0, preedit: std::ptr::null_mut() },
            menu: RimeMenu { page_size: 0, page_no: 0, is_last_page: 0, highlighted_candidate_index: 0, num_candidates: 0, candidates: std::ptr::null_mut(), select_keys: std::ptr::null_mut() },
            commit_text_preview: std::ptr::null_mut(),
            select_labels: std::ptr::null_mut(),
        };
        assert_eq!(RimeGetContext(id, &mut ctx), TRUE);
        assert_eq!(ctx.menu.num_candidates, 1);
        let cand = unsafe { (*ctx.menu.candidates).text };
        let cand_text = unsafe { CStr::from_ptr(cand) }.to_string_lossy().into_owned();
        assert_eq!(cand_text, "你好");
        RimeFreeContext(&mut ctx);

        // commit and read commit text
        assert_eq!(RimeCommitComposition(id), TRUE);
        let mut commit = RimeCommit { data_size: 0, text: std::ptr::null_mut() };
        assert_eq!(RimeGetCommit(id, &mut commit), TRUE);
        let commit_text = unsafe { CStr::from_ptr(commit.text) }.to_string_lossy().into_owned();
        assert_eq!(commit_text, "你好");
        RimeFreeCommit(&mut commit);

        // option round-trip
        RimeSetOption(id, c"ascii_mode".as_ptr(), TRUE);
        assert_eq!(RimeGetOption(id, c"ascii_mode".as_ptr()), TRUE);

        assert_eq!(RimeDestroySession(id), TRUE);
    }

    #[test]
    fn config_roundtrip() {
        let mut map = BTreeMap::new();
        map.insert(
            "menu".to_string(),
            ConfigItem::Map(BTreeMap::from([(
                "page_size".to_string(),
                ConfigItem::Scalar("5".to_string()),
            )])),
        );
        register_config("test", Config::new(ConfigItem::Map(map)));

        let mut config = RimeConfig { ptr: std::ptr::null_mut() };
        assert_eq!(RimeConfigOpen(c"test".as_ptr(), &mut config), TRUE);

        let mut buf = [0u8; 16];
        assert_eq!(
            RimeConfigGetString(&mut config, c"menu/page_size".as_ptr(), buf.as_mut_ptr() as *mut c_char, buf.len()),
            TRUE
        );
        assert_eq!(&buf[..1], b"5");

        // set int
        assert_eq!(RimeConfigSetInt(&mut config, c"menu/page_size".as_ptr(), 10), TRUE);
        let mut val = 0i32;
        assert_eq!(RimeConfigGetInt(&mut config, c"menu/page_size".as_ptr(), &mut val), TRUE);
        assert_eq!(val, 10);

        // set/get bool
        assert_eq!(RimeConfigSetBool(&mut config, c"flag".as_ptr(), TRUE), TRUE);
        let mut b = FALSE;
        assert_eq!(RimeConfigGetBool(&mut config, c"flag".as_ptr(), &mut b), TRUE);
        assert_eq!(b, TRUE);

        // load from YAML
        assert_eq!(RimeConfigLoadString(&mut config, c"a:\n  b: c\n".as_ptr()), TRUE);
        let mut buf2 = [0u8; 8];
        assert_eq!(
            RimeConfigGetString(&mut config, c"a/b".as_ptr(), buf2.as_mut_ptr() as *mut c_char, buf2.len()),
            TRUE
        );
        assert_eq!(&buf2[..1], b"c");

        RimeConfigClose(&mut config);
    }
}
