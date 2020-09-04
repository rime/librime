@0xde912f558dde6b99;

struct RimeCommit {
  # Text to commit to input field.
  text @0 :Text;
}

struct RimeCandidate {
  text @0 :Text;
  comment @1 :Text;
}

struct RimeContext {
  # Input context.

  struct Composition {
    # (de-)Compostion of current input.
    length @0 :Int32;
    cursorPos @1 :Int32;
    selStart @2 :Int32;
    selEnd @3 :Int32;
    preedit @4 :Text;
    commitTextPreview @5 :Text;
  }

  struct Menu {
    # Menu of text candidates.
    pageSize @0 :Int32;
    pageNo @1 :Int32;
    isLastPage @2 :Bool;
    highlightedCandidateIndex @3 :Int32;
    candidates @4 :List(RimeCandidate);
    selectKeys @5 :Text;
    selectLabels @6 :List(Text);
  }

  composition @0 :Composition;
  menu @1 :Menu;
}

struct RimeStatus {
  schemaId @0 :Text;
  schemaName @1 :Text;
  isDisabled @2 :Bool;
  isComposing @3 :Bool;
  isAsciiMode @4 :Bool;
  isFullShape @5 :Bool;
  isSimplified @6 :Bool;
  isTraditional @7 :Bool;
  isAsciiPunct @8 :Bool;
}
