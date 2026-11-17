// Copyright RIME Developers
// Distributed under the BSD License
//
// A small self-contained regular-expression engine, covering the subset of
// boost::regex used by librime's spelling algebra: literals, `.`, `^`, `$`,
// character classes `[...]`/`[^...]`, capture groups `(...)`, quantifiers
// `*`/`+`/`?` (greedy), alternation `|`, and `$N`/`$&` back-references in the
// replacement. This substitutes for the `regex` crate where no network access
// is available; it can be swapped for the crate later.

#[derive(Debug, Clone)]
enum Node {
    Empty,
    Char(char),
    Any,
    Class { negated: bool, ranges: Vec<(char, char)> },
    Start,
    End,
    Concat(Vec<Node>),
    Alt(Vec<Node>),
    Group(usize, Box<Node>),
    Repeat { inner: Box<Node>, min: usize, max: Option<usize> },
}

type Captures = Vec<Option<(usize, usize)>>;

/// A compiled regular expression.
#[derive(Debug, Clone)]
pub struct Regex {
    root: Node,
    group_count: usize,
}

impl Regex {
    pub fn compile(pattern: &str) -> Result<Regex, String> {
        let chars: Vec<char> = pattern.chars().collect();
        let mut parser = Parser {
            chars,
            pos: 0,
            group_count: 0,
        };
        let root = parser.parse_alt()?;
        if parser.pos != parser.chars.len() {
            return Err(format!("unexpected '{}'", parser.chars[parser.pos]));
        }
        Ok(Regex {
            root,
            group_count: parser.group_count,
        })
    }

    fn captures_buffer(&self) -> Captures {
        vec![None; self.group_count + 1]
    }

    /// Find the leftmost match; returns (start, end, captures).
    pub fn find_first(&self, text: &str) -> Option<(usize, usize, Captures)> {
        let chars: Vec<char> = text.chars().collect();
        for start in 0..=chars.len() {
            let caps = self.captures_buffer();
            let results = match_node(&self.root, &chars, start, &caps);
            if let Some((end, captures)) = results.into_iter().next() {
                return Some((start, end, captures));
            }
        }
        None
    }

    pub fn is_match(&self, text: &str) -> bool {
        self.find_first(text).is_some()
    }

    /// Whether the pattern matches the whole string (boost::regex_match).
    pub fn is_full_match(&self, text: &str) -> bool {
        let chars: Vec<char> = text.chars().collect();
        let caps = self.captures_buffer();
        let results = match_node(&self.root, &chars, 0, &caps);
        results.into_iter().any(|(end, _)| end == chars.len())
    }

    /// Replace all non-overlapping matches (boost::regex_replace default).
    pub fn replace_all(&self, text: &str, replacement: &str) -> String {
        let chars: Vec<char> = text.chars().collect();
        let mut result = String::new();
        let mut search_from = 0usize;
        let mut last_end = 0usize;

        while search_from <= chars.len() {
            let mut matched = false;
            for start in search_from..=chars.len() {
                let caps = self.captures_buffer();
                if let Some((end, captures)) =
                    match_node(&self.root, &chars, start, &caps).into_iter().next()
                {
                    result.push_str(&chars[last_end..start].iter().collect::<String>());
                    result.push_str(&expand(replacement, &chars, &captures));
                    last_end = end;
                    matched = true;
                    if end == start {
                        search_from = start + 1;
                    } else {
                        search_from = end;
                    }
                    break;
                }
            }
            if !matched {
                break;
            }
        }
        result.push_str(&chars[last_end..].iter().collect::<String>());
        result
    }
}

fn expand(replacement: &str, chars: &[char], captures: &Captures) -> String {
    let mut result = String::new();
    let mut it = replacement.chars().peekable();
    while let Some(c) = it.next() {
        if c == '$' {
            match it.peek().copied() {
                Some('$') => {
                    result.push('$');
                    it.next();
                }
                Some('&') => {
                    if let Some(Some((s, e))) = captures.get(0) {
                        result.push_str(&chars[*s..*e].iter().collect::<String>());
                    }
                    it.next();
                }
                Some(d) if d.is_ascii_digit() => {
                    let mut idx = 0usize;
                    while let Some(&d) = it.peek() {
                        if d.is_ascii_digit() {
                            idx = idx * 10 + (d as usize - '0' as usize);
                            it.next();
                        } else {
                            break;
                        }
                    }
                    if let Some(Some((s, e))) = captures.get(idx) {
                        result.push_str(&chars[*s..*e].iter().collect::<String>());
                    }
                }
                _ => result.push('$'),
            }
        } else {
            result.push(c);
        }
    }
    result
}

fn match_node(node: &Node, chars: &[char], pos: usize, caps: &Captures) -> Vec<(usize, Captures)> {
    match node {
        Node::Empty => vec![(pos, caps.clone())],
        Node::Char(c) => {
            if chars.get(pos) == Some(c) {
                vec![(pos + 1, caps.clone())]
            } else {
                vec![]
            }
        }
        Node::Any => {
            if pos < chars.len() {
                vec![(pos + 1, caps.clone())]
            } else {
                vec![]
            }
        }
        Node::Start => {
            if pos == 0 {
                vec![(pos, caps.clone())]
            } else {
                vec![]
            }
        }
        Node::End => {
            if pos == chars.len() {
                vec![(pos, caps.clone())]
            } else {
                vec![]
            }
        }
        Node::Class { negated, ranges } => match chars.get(pos) {
            Some(c) => {
                let inside = ranges.iter().any(|&(lo, hi)| *c >= lo && *c <= hi);
                if inside != *negated {
                    vec![(pos + 1, caps.clone())]
                } else {
                    vec![]
                }
            }
            None => vec![],
        },
        Node::Concat(nodes) => {
            let mut results = vec![(pos, caps.clone())];
            for n in nodes {
                let mut next = Vec::new();
                for (p, c) in results {
                    next.extend(match_node(n, chars, p, &c));
                }
                results = next;
            }
            results
        }
        Node::Alt(alts) => {
            let mut results = Vec::new();
            for alt in alts {
                results.extend(match_node(alt, chars, pos, caps));
            }
            results
        }
        Node::Group(idx, inner) => match_node(inner, chars, pos, caps)
            .into_iter()
            .map(|(end, mut c)| {
                c[*idx] = Some((pos, end));
                (end, c)
            })
            .collect(),
        Node::Repeat { inner, min, max } => {
            let hard_max = max.unwrap_or(usize::MAX);
            let mut all: Vec<(usize, Captures, usize)> = Vec::new();
            fn dfs(
                inner: &Node,
                chars: &[char],
                pos: usize,
                caps: &Captures,
                count: usize,
                min: usize,
                hard_max: usize,
                all: &mut Vec<(usize, Captures, usize)>,
            ) {
                if count >= min {
                    all.push((pos, caps.clone(), count));
                }
                if count < hard_max {
                    for (p, c) in match_node(inner, chars, pos, caps) {
                        if p == pos {
                            continue; // avoid infinite loop on empty match
                        }
                        dfs(inner, chars, p, &c, count + 1, min, hard_max, all);
                    }
                }
            }
            dfs(inner, chars, pos, caps, 0, *min, hard_max, &mut all);
            // greedy: longest match first
            all.sort_by(|a, b| b.2.cmp(&a.2));
            all.into_iter().map(|(e, c, _)| (e, c)).collect()
        }
    }
}

struct Parser {
    chars: Vec<char>,
    pos: usize,
    group_count: usize,
}

impl Parser {
    fn peek(&self) -> Option<char> {
        self.chars.get(self.pos).copied()
    }

    fn parse_alt(&mut self) -> Result<Node, String> {
        let mut alternatives = vec![self.parse_concat()?];
        while self.peek() == Some('|') {
            self.pos += 1;
            alternatives.push(self.parse_concat()?);
        }
        if alternatives.len() == 1 {
            Ok(alternatives.pop().unwrap())
        } else {
            Ok(Node::Alt(alternatives))
        }
    }

    fn parse_concat(&mut self) -> Result<Node, String> {
        let mut nodes = Vec::new();
        while let Some(c) = self.peek() {
            if c == '|' || c == ')' {
                break;
            }
            nodes.push(self.parse_repeat()?);
        }
        match nodes.len() {
            0 => Ok(Node::Empty),
            1 => Ok(nodes.pop().unwrap()),
            _ => Ok(Node::Concat(nodes)),
        }
    }

    fn parse_repeat(&mut self) -> Result<Node, String> {
        let atom = self.parse_atom()?;
        match self.peek() {
            Some('*') => {
                self.pos += 1;
                Ok(Node::Repeat { inner: Box::new(atom), min: 0, max: None })
            }
            Some('+') => {
                self.pos += 1;
                Ok(Node::Repeat { inner: Box::new(atom), min: 1, max: None })
            }
            Some('?') => {
                self.pos += 1;
                Ok(Node::Repeat { inner: Box::new(atom), min: 0, max: Some(1) })
            }
            _ => Ok(atom),
        }
    }

    fn parse_atom(&mut self) -> Result<Node, String> {
        let c = self.peek().ok_or("unexpected end of pattern")?;
        match c {
            '.' => {
                self.pos += 1;
                Ok(Node::Any)
            }
            '^' => {
                self.pos += 1;
                Ok(Node::Start)
            }
            '$' => {
                self.pos += 1;
                Ok(Node::End)
            }
            '[' => self.parse_class(),
            '(' => {
                self.pos += 1;
                self.group_count += 1;
                let idx = self.group_count;
                let inner = self.parse_alt()?;
                if self.peek() != Some(')') {
                    return Err("unbalanced parenthesis".to_string());
                }
                self.pos += 1;
                Ok(Node::Group(idx, Box::new(inner)))
            }
            '\\' => {
                self.pos += 1;
                let escaped = self
                    .peek()
                    .ok_or("trailing backslash")?;
                self.pos += 1;
                Ok(Node::Char(escaped))
            }
            '*' | '+' | '?' | '|' | ')' => Err(format!("unexpected '{}'", c)),
            _ => {
                self.pos += 1;
                Ok(Node::Char(c))
            }
        }
    }

    fn parse_class(&mut self) -> Result<Node, String> {
        self.pos += 1; // consume '['
        let mut negated = false;
        if self.peek() == Some('^') {
            negated = true;
            self.pos += 1;
        }
        let mut ranges = Vec::new();
        let mut first = true;
        loop {
            let c = self.peek().ok_or("unterminated character class")?;
            if c == ']' && !first {
                self.pos += 1;
                break;
            }
            first = false;
            let lo = if c == '\\' {
                self.pos += 1;
                self.peek().ok_or("trailing backslash in class")?
            } else {
                c
            };
            self.pos += 1;
            // range?
            if self.peek() == Some('-') && self.chars.get(self.pos + 1) != Some(&']') {
                self.pos += 1;
                let hi = self.peek().ok_or("unterminated character class")?;
                self.pos += 1;
                ranges.push((lo, hi));
            } else {
                ranges.push((lo, lo));
            }
        }
        Ok(Node::Class { negated, ranges })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn literal_and_anchors() {
        let re = Regex::compile("^([zcs])h(.*)$").unwrap();
        let m = re.find_first("shang").unwrap();
        assert_eq!(m.0, 0);
        assert_eq!(m.1, 5);
        assert_eq!(re.replace_all("shang", "$1$2"), "sang");
        assert!(!re.is_match("bang"));
    }

    #[test]
    fn char_class_and_quantifier() {
        let re = Regex::compile("^[czs]h[aoe]ng?$").unwrap();
        assert!(re.is_full_match("shang"));
        assert!(re.is_full_match("zhong"));
        assert!(re.is_full_match("chen"));
        assert!(!re.is_full_match("bang"));
    }

    #[test]
    fn transliteration_pattern_roundtrip() {
        // back-reference duplication
        let re = Regex::compile("^([a-z])$").unwrap();
        assert_eq!(re.replace_all("a", "$1$1"), "aa");
    }

    #[test]
    fn alternation() {
        let re = Regex::compile("^(ab|cd)$").unwrap();
        assert!(re.is_full_match("ab"));
        assert!(re.is_full_match("cd"));
        assert!(!re.is_full_match("ac"));
    }

    #[test]
    fn dot_star_greedy() {
        let re = Regex::compile("^a(.*)b$").unwrap();
        assert_eq!(re.replace_all("axxb", "$1"), "xx");
        // greedy: (.*) takes the longest match
        assert_eq!(re.replace_all("axxbxb", "$1"), "xxbx");
    }
}
