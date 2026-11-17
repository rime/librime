// Copyright RIME Developers
// Distributed under the BSD License
//
// Port of src/rime/candidate.h and src/rime/candidate.cc to Rust.

use std::sync::Arc;

/// Shared reference to a candidate (mirrors `an<Candidate>` / shared_ptr).
pub type CandidateArc = Arc<dyn Candidate>;
/// Ordered list of candidates (mirrors `CandidateList`).
pub type CandidateList = Vec<CandidateArc>;

/// Common mutable candidate attributes (type, range, quality).
#[derive(Debug, Clone, Default)]
pub struct CandidateBase {
    pub type_: String,
    pub start: usize,
    pub end: usize,
    pub quality: f64,
}

/// A candidate produced by a translator.
pub trait Candidate: Send + Sync {
    fn base(&self) -> &CandidateBase;
    fn base_mut(&mut self) -> &mut CandidateBase;

    // recognized by translators in learning phase
    fn type_name(&self) -> &str {
        &self.base().type_
    }
    // [start, end) marks a range in the input that the candidate corresponds to
    fn start(&self) -> usize {
        self.base().start
    }
    fn end(&self) -> usize {
        self.base().end
    }
    fn quality(&self) -> f64 {
        self.base().quality
    }

    // candidate text to commit
    fn text(&self) -> &str;
    // (optional)
    fn comment(&self) -> &str {
        ""
    }
    // text shown in the preedit area, replacing input string (optional)
    fn preedit(&self) -> &str {
        ""
    }

    fn set_type(&mut self, t: &str) {
        self.base_mut().type_ = t.to_string();
    }
    fn set_start(&mut self, s: usize) {
        self.base_mut().start = s;
    }
    fn set_end(&mut self, e: usize) {
        self.base_mut().end = e;
    }
    fn set_quality(&mut self, q: f64) {
        self.base_mut().quality = q;
    }

    /// Clone as a `Arc<dyn Candidate>` (replaces C++ dynamic_pointer_cast).
    fn clone_candidate(&self) -> CandidateArc;

    // dynamic_cast stand-ins used by genuine_candidate(s)
    fn as_shadow_item(&self) -> Option<CandidateArc> {
        None
    }
    fn as_uniquified_items(&self) -> Option<Vec<CandidateArc>> {
        None
    }
}

/// Compare two candidates (mirrors `Candidate::compare`).
/// Returns < 0 if `a` sorts before `b`, > 0 if after, 0 if equal.
pub fn compare(a: &dyn Candidate, b: &dyn Candidate) -> i32 {
    // the one nearer to the beginning of segment comes first
    let k = (a.start() as i64) - (b.start() as i64);
    if k != 0 {
        return k as i32;
    }
    // then the longer comes first
    let k = (a.end() as i64) - (b.end() as i64);
    if k != 0 {
        return -(k as i32);
    }
    // compare quality
    let qdiff = a.quality() - b.quality();
    if qdiff != 0.0 {
        return if qdiff > 0.0 { -1 } else { 1 };
    }
    0
}

fn unpack_shadow(cand: &CandidateArc) -> CandidateArc {
    match cand.as_shadow_item() {
        Some(item) => item,
        None => Arc::clone(cand),
    }
}

/// The genuine (underlying) candidate, unpacking one level of shadowing and
/// taking the front item of a uniquified candidate.
pub fn genuine_candidate(cand: &CandidateArc) -> CandidateArc {
    if let Some(items) = cand.as_uniquified_items() {
        if let Some(first) = items.first() {
            return unpack_shadow(first);
        }
    }
    unpack_shadow(cand)
}

/// All genuine candidates for a (possibly uniquified) candidate.
pub fn genuine_candidates(cand: &CandidateArc) -> Vec<CandidateArc> {
    if let Some(items) = cand.as_uniquified_items() {
        items.iter().map(unpack_shadow).collect()
    } else {
        vec![unpack_shadow(cand)]
    }
}

/// A candidate with explicit text/comment/preedit.
#[derive(Debug, Clone)]
pub struct SimpleCandidate {
    base: CandidateBase,
    text: String,
    comment: String,
    preedit: String,
}

impl SimpleCandidate {
    pub fn new(
        type_: &str,
        start: usize,
        end: usize,
        text: &str,
        comment: &str,
        preedit: &str,
    ) -> Self {
        Self {
            base: CandidateBase {
                type_: type_.to_string(),
                start,
                end,
                quality: 0.0,
            },
            text: text.to_string(),
            comment: comment.to_string(),
            preedit: preedit.to_string(),
        }
    }

    pub fn set_text(&mut self, text: &str) {
        self.text = text.to_string();
    }
    pub fn set_comment(&mut self, comment: &str) {
        self.comment = comment.to_string();
    }
    pub fn set_preedit(&mut self, preedit: &str) {
        self.preedit = preedit.to_string();
    }
}

impl Candidate for SimpleCandidate {
    fn base(&self) -> &CandidateBase {
        &self.base
    }
    fn base_mut(&mut self) -> &mut CandidateBase {
        &mut self.base
    }
    fn text(&self) -> &str {
        &self.text
    }
    fn comment(&self) -> &str {
        &self.comment
    }
    fn preedit(&self) -> &str {
        &self.preedit
    }
    fn clone_candidate(&self) -> CandidateArc {
        Arc::new(self.clone())
    }
}

/// Wraps another candidate with an override for type/text/comment.
#[derive(Clone)]
pub struct ShadowCandidate {
    base: CandidateBase,
    text: String,
    comment: String,
    item: CandidateArc,
    inherit_comment: bool,
}

impl ShadowCandidate {
    pub fn new(
        item: CandidateArc,
        type_: &str,
        text: &str,
        comment: &str,
        inherit_comment: bool,
    ) -> Self {
        Self {
            base: CandidateBase {
                type_: type_.to_string(),
                start: item.start(),
                end: item.end(),
                quality: item.quality(),
            },
            text: text.to_string(),
            comment: comment.to_string(),
            item,
            inherit_comment,
        }
    }

    pub fn item(&self) -> &CandidateArc {
        &self.item
    }
}

impl Candidate for ShadowCandidate {
    fn base(&self) -> &CandidateBase {
        &self.base
    }
    fn base_mut(&mut self) -> &mut CandidateBase {
        &mut self.base
    }
    fn text(&self) -> &str {
        if self.text.is_empty() {
            self.item.text()
        } else {
            &self.text
        }
    }
    fn comment(&self) -> &str {
        if self.inherit_comment && self.comment.is_empty() {
            self.item.comment()
        } else {
            &self.comment
        }
    }
    fn preedit(&self) -> &str {
        self.item.preedit()
    }
    fn clone_candidate(&self) -> CandidateArc {
        Arc::new(self.clone())
    }
    fn as_shadow_item(&self) -> Option<CandidateArc> {
        Some(Arc::clone(&self.item))
    }
}

/// Groups several candidates that share the same text.
#[derive(Clone)]
pub struct UniquifiedCandidate {
    base: CandidateBase,
    text: String,
    comment: String,
    items: Vec<CandidateArc>,
}

impl UniquifiedCandidate {
    pub fn new(item: CandidateArc, type_: &str, text: &str, comment: &str) -> Self {
        let mut c = Self {
            base: CandidateBase {
                type_: type_.to_string(),
                start: item.start(),
                end: item.end(),
                quality: item.quality(),
            },
            text: text.to_string(),
            comment: comment.to_string(),
            items: Vec::new(),
        };
        c.append(item);
        c
    }

    pub fn append(&mut self, item: CandidateArc) {
        if self.base.quality < item.quality() {
            self.base.quality = item.quality();
        }
        self.items.push(item);
    }

    pub fn items(&self) -> &[CandidateArc] {
        &self.items
    }
}

impl Candidate for UniquifiedCandidate {
    fn base(&self) -> &CandidateBase {
        &self.base
    }
    fn base_mut(&mut self) -> &mut CandidateBase {
        &mut self.base
    }
    fn text(&self) -> &str {
        if self.text.is_empty() {
            self.items.first().map(|c| c.text()).unwrap_or(&self.text)
        } else {
            &self.text
        }
    }
    fn comment(&self) -> &str {
        if self.comment.is_empty() {
            self.items.first().map(|c| c.comment()).unwrap_or(&self.comment)
        } else {
            &self.comment
        }
    }
    fn preedit(&self) -> &str {
        self.items.first().map(|c| c.preedit()).unwrap_or("")
    }
    fn clone_candidate(&self) -> CandidateArc {
        Arc::new(self.clone())
    }
    fn as_uniquified_items(&self) -> Option<Vec<CandidateArc>> {
        Some(self.items.clone())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn simple(text: &str, start: usize, end: usize) -> CandidateArc {
        Arc::new(SimpleCandidate::new("simple", start, end, text, "", ""))
    }

    #[test]
    fn compare_orders_by_start_then_length_then_quality() {
        let a = simple("a", 0, 1);
        let b = simple("b", 0, 2);
        // same start, longer first
        assert!(compare(a.as_ref(), b.as_ref()) > 0);

        let c = simple("c", 1, 2);
        // earlier start first
        assert!(compare(a.as_ref(), c.as_ref()) < 0);
    }

    #[test]
    fn shadow_unpacks_to_item() {
        let item = simple("item", 0, 1);
        let shadow: CandidateArc = Arc::new(ShadowCandidate::new(
            Arc::clone(&item),
            "shadow",
            "",
            "",
            true,
        ));
        assert_eq!(shadow.text(), "item");
        assert_eq!(genuine_candidate(&shadow).text(), "item");
    }

    #[test]
    fn uniquified_exposes_items() {
        let a = simple("你好", 0, 2);
        let b = simple("您好", 0, 2);
        let u: CandidateArc = Arc::new(UniquifiedCandidate::new(Arc::clone(&a), "uniq", "", ""));
        // append via as_uniquified_items is awkward; construct directly instead
        drop(u);
        let mut u = UniquifiedCandidate::new(Arc::clone(&a), "uniq", "", "");
        u.append(Arc::clone(&b));
        let u: CandidateArc = Arc::new(u);
        assert_eq!(u.text(), "你好");
        let genuine = genuine_candidates(&u);
        assert_eq!(genuine.len(), 2);
    }
}
