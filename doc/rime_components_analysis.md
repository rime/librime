# RIME Documentation

> This document is generated through AI by @harlos0517 and has not been thorougly reviewed by any human developer. I do not guarantee the accuracy of the information provided.

> This document is WIP. Feel free to give edit suggestions or corrections in the comments section.

This document provides a detailed analysis of the components (gears) in the Rime Input Method Engine and how their configuration options affect functionality.

## Table of Contents
1. [Overview of Rime Architecture](#overview-of-rime-architecture)
2. [Segmentors](#segmentors)
3. [Composers](#composers)
4. [Translators](#translators)
5. [Filters](#filters)
6. [Processors](#processors)
7. [Other Components](#other-components)
8. [Configuration Options Reference](#configuration-options-reference)

## Overview of Rime Architecture

Rime is a modular input method engine that processes keystrokes through a pipeline of components:

1. **Processors**: Handle raw key events (e.g., ascii_composer)
2. **Segmentors**: Break input into meaningful segments (e.g., abc_segmentor)
3. **Translators**: Convert segments into candidates (e.g., table_translator)
4. **Filters**: Refine candidate lists (e.g., uniquifier)![alt text](image.png)

Each component can be configured through schema files using the YAML format.

## Segmentors

Segmentors divide the input string into meaningful segments for further processing.

### abc_segmentor

**Purpose**: Segments alphabetic input according to spelling rules.

**Configuration Options**:
- `extra_tags`: Additional tags to attach to segments created by this segmentor.
  - Type: List of strings
  - Effect: Tags influence how other components process these segments.

**Implementation Details**:
- Uses `speller/alphabet`, `speller/delimiter`, `speller/initials`, and `speller/finals` from the schema configuration.
- Creates segments with the "abc" tag plus any extra_tags specified.
- Segments input based on valid initial and final character combinations.

### ascii_segmentor

**Purpose**: Identifies ASCII text segments that should be processed differently from other input.

**Implementation Details**:
- Creates segments with the "ascii" tag.
- No specific configuration options in the config_structure.yaml.

### affix_segmentor

**Purpose**: Handles prefixes and suffixes in the input.

**Implementation Details**:
- Configured through `affix_segmentor/tag`, `affix_segmentor/prefix`, and `affix_segmentor/suffix`.
- Allows special handling of input with specific prefixes or suffixes.

### fallback_segmentor

**Purpose**: Provides fallback segmentation when other segmentors fail.

**Implementation Details**:
- Creates a segment for the whole input string when no other segmentation is available.
- No specific configuration options in the config_structure.yaml.

## Composers

Composers handle raw key events and convert them into input strings.

### ascii_composer

**Purpose**: Handles ASCII mode switching and composition.

**Configuration Options**:
- `good_old_caps_lock`: When true, treats CapsLock as a mode switch rather than a modifier key.
  - Type: Boolean
  - Effect: Changes how CapsLock behaves in the input method.
- `switch_key`: Defines key bindings for switching between Chinese and ASCII input modes.
  - Type: Map of key names to switch styles
  - Effect: Controls how and when the input method switches between Chinese and ASCII modes.

**Implementation Details**:
- Processes key events before they reach other components.
- Manages ASCII mode toggling based on configured keys.
- Switch styles include: inline_ascii, commit_text, commit_code, clear, set_ascii_mode, unset_ascii_mode.

### chord_composer

**Purpose**: Enables chord-typing input methods where multiple keys are pressed simultaneously.

**Configuration Options**:
- `alphabet`: The set of valid characters for chord input.
  - Type: String
  - Effect: Defines which keys can be used in chord combinations.
- `algebra`: Spelling algebra rules for chord-typing input transformation.
  - Type: List of rules
  - Effect: Transforms chord input according to specified rules.
- `output_format`: Rules for formatting the output of chord compositions.
  - Type: List of formatting rules
  - Effect: Determines how chord input is presented as output.
- `prompt_format`: Rules for formatting the prompt displayed during chord input.
  - Type: List of formatting rules
  - Effect: Controls how the input prompt appears during chord typing.
- `use_alt`, `use_caps`, `use_control`, `use_shift`, `use_super`: Whether to use modifier keys in chord combinations.
  - Type: Boolean
  - Effect: Determines which modifier keys participate in chord input.
- `finish_chord_on_first_key_release`: When true, commits chord on first key release.
  - Type: Boolean
  - Effect: Changes when chord input is committed - either on first key release or when all keys are released.

**Implementation Details**:
- Tracks simultaneous key presses and combines them according to rules.
- Particularly useful for specialized input methods like Combo Pinyin.
- Uses the speller's delimiter configuration.

## Translators

Translators convert segmented input into candidate lists.

### translator (base class)

**Configuration Options**:
- `dictionary`: Dictionary file used by the translator.
  - Type: String
  - Effect: Specifies which dictionary file to use for lookups.

### table_translator

**Purpose**: Translates input based on a table dictionary.

**Implementation Details**:
- Performs dictionary lookups for input segments.
- Supports user dictionaries and custom vocabulary.
- Configuration includes `translator/dictionary`, `translator/enable_user_dict`, etc.

### script_translator

**Purpose**: Specialized translator for phonetic input methods.

**Implementation Details**:
- Extends table_translator with features specific to phonetic input.
- Handles spelling corrections and fuzzy matching.

### echo_translator

**Purpose**: Simply echoes the input as a candidate.

**Implementation Details**:
- Creates a candidate that is identical to the input.
- Useful as a fallback when no translation is available.

### reverse_lookup_translator

**Purpose**: Provides reverse lookups in another dictionary.

**Implementation Details**:
- Allows looking up characters by alternative means (e.g., by Pinyin when using a shape-based input method).
- Configured through `reverse_lookup/dictionary`, etc.

### schema_list_translator

**Purpose**: Translates schema list selections.

**Implementation Details**:
- Used by the switcher to display and select available schemas.

### switch_translator

**Purpose**: Translates option switches.

**Implementation Details**:
- Used by the switcher to display and toggle options.

## Filters

Filters refine candidate lists by applying various transformations or filtering rules.

### uniquifier

**Purpose**: Removes duplicate candidates.

**Implementation Details**:
- Ensures each candidate appears only once in the list.

### single_char_filter

**Purpose**: Filters to show only single-character candidates.

**Implementation Details**:
- Useful for certain input scenarios where only single characters are desired.

### reverse_lookup_filter

**Purpose**: Annotates candidates with reverse lookup results.

**Implementation Details**:
- Adds annotations to candidates based on a reverse lookup dictionary.
- Configured through `reverse_lookup/dictionary`, etc.

### charset_filter

**Purpose**: Filters candidates based on character set.

**Implementation Details**:
- Can filter out characters not in a specified character set.
- Useful for restricting output to specific character sets (e.g., Traditional or Simplified Chinese).

## Processors

Processors handle key events and control the input method's behavior.

### key_binder

**Purpose**: Binds key combinations to specific actions.

**Configuration Options**:
- `bindings`: Custom key bindings for various actions.
  - Type: List of key-action pairs
  - Effect: Defines keyboard shortcuts for various input method functions.

**Implementation Details**:
- Processes key events and triggers corresponding actions.
- Supports various action types like commit, move, select, etc.

### speller

**Purpose**: Handles spelling behavior and auto-selection.

**Configuration Options**:
- `alphabet`: Set of valid characters for spelling.
  - Type: String
  - Effect: Defines which characters are valid in the input.
- `delimiter`: Character used as delimiter between syllables.
  - Type: String
  - Effect: Specifies how syllables are separated in the input.
- `initials`: Set of valid initial characters in syllables.
  - Type: String
  - Effect: Defines which characters can start a syllable.
- `finals`: Set of valid final characters in syllables.
  - Type: String
  - Effect: Defines which characters can end a syllable.
- `auto_select`: Whether to automatically select candidates.
  - Type: Boolean
  - Effect: When true, automatically selects candidates in certain situations.
- `auto_select_pattern`: Pattern that triggers auto-selection of candidates.
  - Type: String
  - Effect: Defines when auto-selection should occur based on input patterns.
- `auto_clear`: When to automatically clear input.
  - Type: String
  - Effect: Controls when input is automatically cleared.
- `max_code_length`: Maximum length of input code.
  - Type: Integer
  - Effect: Limits the length of input code.
- `use_space`: Whether to use space as a delimiter.
  - Type: Boolean
  - Effect: Determines if space key acts as a delimiter.

**Implementation Details**:
- Controls how input is interpreted and when candidates are automatically selected.
- Works closely with segmentors to determine valid input sequences.

### punctuator

**Purpose**: Handles punctuation input and conversion.

**Configuration Options**:
- `symbols`: Mapping of punctuation symbols.
  - Type: Map of keys to punctuation symbols
  - Effect: Defines how punctuation keys are mapped to actual punctuation symbols.
- `use_space`: Whether to use space as a punctuation key.
  - Type: Boolean
  - Effect: Determines if space can trigger punctuation in certain contexts.
- `digit_separators`: Characters that separate digits.
  - Type: String
  - Effect: Defines which characters are considered digit separators.
- `digit_separator_action`: Action to take when a digit separator is entered.
  - Type: String
  - Effect: Controls behavior when a digit separator is typed (e.g., "commit").

**Implementation Details**:
- Converts punctuation keys to appropriate symbols based on context.
- Handles full-width and half-width punctuation.
- Manages special punctuation behavior like paired symbols.

### selector

**Purpose**: Handles candidate selection.

**Implementation Details**:
- Processes number keys and other selection keys.
- Works with the menu component to select candidates.

### navigator

**Purpose**: Handles navigation through candidate lists.

**Implementation Details**:
- Processes arrow keys and page up/down for navigating candidates.
- Works with the menu component.

### editor

**Purpose**: Handles text editing operations.

**Configuration Options**:
- `char_handler`: Handler for character-level editing operations.
  - Type: String or null
  - Effect: Specifies a custom handler for character editing operations.

**Implementation Details**:
- Processes editing keys like Backspace, Delete, Home, End, etc.
- Modifies the composition state accordingly.

## Other Components

### menu

**Purpose**: Manages the candidate menu display.

**Configuration Options**:
- `page_size`: Number of candidates to display per page.
  - Type: Integer
  - Effect: Controls how many candidates are shown at once.
- `page_down_cycle`: When true, paging down at the last page cycles back to the first page.
  - Type: Boolean
  - Effect: Determines navigation behavior at the end of the candidate list.
- `alternative_select_keys`: Keys used for selecting candidates other than number keys.
  - Type: String
  - Effect: Provides alternative keys for candidate selection.
- `alternative_select_labels`: Labels to show for alternative selection keys.
  - Type: List of strings
  - Effect: Customizes the display of selection labels.

**Implementation Details**:
- Manages the display and selection of candidates.
- Handles paging through candidate lists.

### schema

**Purpose**: Defines the overall input schema.

**Configuration Options**:
- `name`: Display name of the input schema.
  - Type: String
  - Effect: Sets the human-readable name of the schema.
- `schema_id`: Unique identifier for the input schema.
  - Type: String
  - Effect: Provides a unique ID for the schema.

**Implementation Details**:
- Contains metadata about the input schema.
- References to the schema are used throughout the system.

### schema_list

**Purpose**: List of available input schemas.

**Configuration Options**:
- Schema list is an array of schema entries.
  - Type: List
  - Effect: Defines which schemas are available for selection.

**Implementation Details**:
- Used by the switcher to display and select schemas.

### switcher

**Purpose**: Manages switching between schemas and toggling options.

**Configuration Options**:
- `caption`: Caption text for the switcher.
  - Type: String
  - Effect: Sets the title displayed in the switcher UI.
- `hotkeys`: Hotkeys for activating the switcher.
  - Type: List of strings
  - Effect: Defines keyboard shortcuts to open the switcher.
- `save_options`: Options to save between sessions.
  - Type: List of strings
  - Effect: Specifies which options should persist across sessions.
- `fold_options`: Whether to group related options together.
  - Type: Boolean
  - Effect: Controls how options are organized in the UI.
- `abbreviate_options`: Whether to show abbreviated option names.
  - Type: Boolean
  - Effect: Controls how option names are displayed.
- `fix_schema_list_order`: Whether to maintain a fixed order for the schema list.
  - Type: Boolean
  - Effect: When true, schemas are always shown in the same order.
- `option_list_prefix`, `option_list_separator`, `option_list_suffix`: Formatting for option lists.
  - Type: String
  - Effect: Controls the formatting of option lists in the UI.

**Implementation Details**:
- Provides a UI for switching between input schemas.
- Allows toggling various options.
- Manages persistence of options across sessions.

### encoder

**Purpose**: Encodes text according to rules.

**Configuration Options**:
- `exclude_patterns`: Patterns to exclude from encoding.
  - Type: List of patterns
  - Effect: Specifies which patterns should not be encoded.
- `rules`: Rules for encoding text.
  - Type: List of rules
  - Effect: Defines how text should be encoded.
- `tail_anchor`: Anchor character for the end of encoded text.
  - Type: String
  - Effect: Specifies a character to mark the end of encoded text.

**Implementation Details**:
- Used for encoding text according to specific rules.
- Particularly relevant for certain specialized input methods.

## Configuration Options Reference

This section provides a comprehensive reference of all configuration options organized by component.

### abc_segmentor
- `extra_tags`: Additional tags to be added to segments created by the abc_segmentor.
  - Type: List of strings
  - Default: Empty list

### ascii_composer
- `good_old_caps_lock`: When true, treats CapsLock as a mode switch rather than a modifier key.
  - Type: Boolean
  - Default: false
- `switch_key`: Defines key bindings for switching between Chinese and ASCII input modes.
  - Type: Map of key names to switch styles
  - Default: Empty map

### chord_composer
- `algebra`: Spelling algebra rules for chord-typing input transformation.
  - Type: List of rules
  - Default: Empty list
- `alphabet`: The set of valid characters for chord input.
  - Type: String
  - Default: Empty string
- `finish_chord_on_first_key_release`: When true, commits chord on first key release.
  - Type: Boolean
  - Default: false
- `output_format`: Rules for formatting the output of chord compositions.
  - Type: List of formatting rules
  - Default: Empty list
- `prompt_format`: Rules for formatting the prompt displayed during chord input.
  - Type: List of formatting rules
  - Default: Empty list
- `use_alt`: Whether to use Alt key in chord combinations.
  - Type: Boolean
  - Default: false
- `use_caps`: Whether to use CapsLock in chord combinations.
  - Type: Boolean
  - Default: false
- `use_control`: Whether to use Control key in chord combinations.
  - Type: Boolean
  - Default: false
- `use_shift`: Whether to use Shift key in chord combinations.
  - Type: Boolean
  - Default: false
- `use_super`: Whether to use Super/Windows key in chord combinations.
  - Type: Boolean
  - Default: false

### editor
- `char_handler`: Handler for character-level editing operations.
  - Type: String or null
  - Default: null

### encoder
- `exclude_patterns`: Patterns to exclude from encoding.
  - Type: List of patterns
  - Default: Empty list
- `rules`: Rules for encoding text.
  - Type: List of rules
  - Default: Empty list
- `tail_anchor`: Anchor character for the end of encoded text.
  - Type: String
  - Default: Empty string

### key_binder
- `bindings`: Custom key bindings for various actions.
  - Type: List of key-action pairs
  - Default: Empty list

### menu
- `alternative_select_keys`: Keys used for selecting candidates other than number keys.
  - Type: String
  - Default: Empty string
- `alternative_select_labels`: Labels to show for alternative selection keys.
  - Type: List of strings
  - Default: Empty list
- `page_down_cycle`: When true, paging down at the last page cycles back to the first page.
  - Type: Boolean
  - Default: false
- `page_size`: Number of candidates to display per page.
  - Type: Integer
  - Default: 0 (uses system default)

### punctuator
- `digit_separator_action`: Action to take when a digit separator is entered.
  - Type: String
  - Default: Empty string
- `digit_separators`: Characters that separate digits.
  - Type: String
  - Default: Empty string
- `symbols`: Mapping of punctuation symbols.
  - Type: Map of keys to punctuation symbols
  - Default: Empty map
- `use_space`: Whether to use space as a punctuation key.
  - Type: Boolean
  - Default: false

### schema
- `name`: Display name of the input schema.
  - Type: String
  - Default: Empty string
- `schema_id`: Unique identifier for the input schema.
  - Type: String
  - Default: Empty string

### schema_list
- List of available input schemas.
  - Type: List
  - Default: Empty list

### speller
- `alphabet`: Set of valid characters for spelling.
  - Type: String
  - Default: Empty string
- `auto_clear`: When to automatically clear input.
  - Type: String
  - Default: Empty string
- `auto_select_pattern`: Pattern that triggers auto-selection of candidates.
  - Type: String
  - Default: Empty string
- `auto_select`: Whether to automatically select candidates.
  - Type: Boolean
  - Default: false
- `delimiter`: Character used as delimiter between syllables.
  - Type: String
  - Default: Empty string
- `finals`: Set of valid final characters in syllables.
  - Type: String
  - Default: Empty string
- `initials`: Set of valid initial characters in syllables.
  - Type: String
  - Default: Empty string
- `max_code_length`: Maximum length of input code.
  - Type: Integer
  - Default: 0 (no limit)
- `use_space`: Whether to use space as a delimiter.
  - Type: Boolean
  - Default: false

### switcher
- `abbreviate_options`: Whether to show abbreviated option names.
  - Type: Boolean
  - Default: false
- `caption`: Caption text for the switcher.
  - Type: String
  - Default: Empty string
- `fix_schema_list_order`: Whether to maintain a fixed order for the schema list.
  - Type: Boolean
  - Default: false
- `fold_options`: Whether to group related options together.
  - Type: Boolean
  - Default: false
- `hotkeys`: Hotkeys for activating the switcher.
  - Type: List of strings
  - Default: Empty list
- `option_list_prefix`: Prefix for option list items.
  - Type: String
  - Default: Empty string
- `option_list_separator`: Separator between option list items.
  - Type: String
  - Default: Empty string
- `option_list_suffix`: Suffix for option list items.
  - Type: String
  - Default: Empty string
- `save_options`: Options to save between sessions.
  - Type: List of strings
  - Default: Empty list

### translator
- `dictionary`: Dictionary file used by the translator.
  - Type: String
  - Default: Empty string
