<a name="1.11.2"></a>
## [1.11.2](https://github.com/rime/librime/compare/1.11.1...1.11.2) (2024-04-27)


### Bug Fixes

* caret_pos is before caret character ([f5df6cd](https://github.com/rime/librime/commit/f5df6cd)), closes [#860](https://github.com/rime/librime/issues/860)
* **switch_translator:** skip switches with no state labels ([ce77835](https://github.com/rime/librime/commit/ce77835)), closes [#866](https://github.com/rime/librime/issues/866) [#640](https://github.com/rime/librime/issues/640)



<a name="1.11.1"></a>
## [1.11.1](https://github.com/rime/librime/compare/1.11.0...1.11.1) (2024-04-25)


### Bug Fixes

* crash on select_schema .default ([ef08da3](https://github.com/rime/librime/commit/ef08da3))
* **script_translator:** correction can cause segfault ([#863](https://github.com/rime/librime/issues/863)) ([feec253](https://github.com/rime/librime/commit/feec253))



# [1.11.0](https://github.com/rime/librime/compare/1.10.0...1.11.0) (2024-03-29)


### Bug Fixes

* **build.bat:** return error if unit test failed ([24fa8ae](https://github.com/rime/librime/commit/24fa8ae8a48b2e5e629cd55090c1537f2cf9e1ab))
* **chord_composer:** ignore repeated keys ([#841](https://github.com/rime/librime/issues/841)) ([e554510](https://github.com/rime/librime/commit/e554510bf2adeafb2383a1208750dc3b03e1427f))
* **context:** tag selected segment after editing ([8b7f6b7](https://github.com/rime/librime/commit/8b7f6b746b4f7790ade50f60c5d4b89488bc8f9d)), closes [#746](https://github.com/rime/librime/issues/746) [#830](https://github.com/rime/librime/issues/830)
* crash by iteration on non-existing directory ([ec2d87d](https://github.com/rime/librime/commit/ec2d87df5a23aa03cef3e46d03c490d87b1e5234))
* **echo_translator:** return null for empty query ([ec4bdfe](https://github.com/rime/librime/commit/ec4bdfe4b9937f790417e8a3aec006d4c5d6dd85))
* format with clang-foramt 18.1 ([2f4c8f0](https://github.com/rime/librime/commit/2f4c8f0c9c4e4153f37206e28c19d9471849e86f))
* **glog:** configure macros ([ab5d1d2](https://github.com/rime/librime/commit/ab5d1d2f74f2141e9c7c6635fddcfd7cc579a942))
* **glog:** no read permission on the cwd on Android ([#835](https://github.com/rime/librime/issues/835)) ([edee320](https://github.com/rime/librime/commit/edee3204f6272fec7d90a09eff1a0336b00b60c3))
* **glog:** replace deprecated API ([20fdcc1](https://github.com/rime/librime/commit/20fdcc19e65955f6f57f33497f3098aab00af34e))
* **glog:** someone at google is to blame for linking error ([49c7298](https://github.com/rime/librime/commit/49c7298b22024dc1b84375ba8795adaf0be63f09))
* **install-boost.bat:** quit without running b2 ([72c67af](https://github.com/rime/librime/commit/72c67aff8042b640cfd721245b11f6ea10831045))
* **install-boost.sh:** exit code 0 ([8b7938b](https://github.com/rime/librime/commit/8b7938b7d0b6b524f16df05671d01359c2ec93bb))
* log rime::path by operator overloading ([060555c](https://github.com/rime/librime/commit/060555c62c0dfbad02538e0fd96eb1a03540abb3))
* **plugins:** load plugins in relative path ([a791879](https://github.com/rime/librime/commit/a79187966ee0328c3635a5c5f6ecb4cf1e3463ee))
* **rime_api:** deploy_schema type cast failure ([51076a8](https://github.com/rime/librime/commit/51076a8ec9ccf227d10a900c95945b50ef3a7d48))
* **switcher:** superfluously load saved options ([7892a4f](https://github.com/rime/librime/commit/7892a4f53d1a2f12706401b07cbd366eaf87596c))
* **test:** copy rime.dll to test dir ([872cecf](https://github.com/rime/librime/commit/872cecf7f92b6afab7618a4b01e2d8c8f7f7bd7c))
* **test:** ctest unable to find executable ([0061e24](https://github.com/rime/librime/commit/0061e24025e467826a3912f634c06fdb62f79e25))


### Features

* **chord_composer:** finish chord on first release ([#828](https://github.com/rime/librime/issues/828)) ([102d421](https://github.com/rime/librime/commit/102d42174c2b2d793096621efa571328525c2ed3))
* **config:** Config::Save saves data if modified ([12f460f](https://github.com/rime/librime/commit/12f460f2d5b702bf0b2798b2e5ddcda72feeaf6b))
* **dict_compiler:** rebuild packs on demand ([#816](https://github.com/rime/librime/issues/816)) ([28d1e7e](https://github.com/rime/librime/commit/28d1e7e22f2bfc02317e0cf308937934d9cd2d6e))
* **dictionary:** predict word ([95cb5fe](https://github.com/rime/librime/commit/95cb5fe91967e28a25cfc7d5b91b343cc9c42fe2))
* **dictionary:** sort predictive match words ([253e0bd](https://github.com/rime/librime/commit/253e0bd2e1e4536109fb43213dc751dbeb196095))
* **script_translator:** preedit for predicted word ([f907369](https://github.com/rime/librime/commit/f90736938cbb3be6fac118b89082fa9f74394524))
* **script_translator:** sentence over prediction ([729aa62](https://github.com/rime/librime/commit/729aa62b0126a5b59f26204fd6789d015bcbc50d))
* **script_translator:** word completion from 2nd place ([#848](https://github.com/rime/librime/issues/848)) ([5c7fb64](https://github.com/rime/librime/commit/5c7fb64be01f4f43f62c8d7dc4bee5d0ac34fed5))
* **user_dictionary:** predict word ([01affef](https://github.com/rime/librime/commit/01affefaafecfc61a3e275a72f7e05260c7f81ee))


### Reverts

* Revert "build(Makefile): fix install targets" ([33ce245](https://github.com/rime/librime/commit/33ce2459f82e2e99ac6232a11424d883a4119894))



<a name="1.10.0"></a>
# [1.10.0](https://github.com/rime/librime/compare/1.9.0...1.10.0) (2024-02-09)


### Bug Fixes

* **chord_composer:** stop at super and caps by default ([8709a7a](https://github.com/rime/librime/commit/8709a7a))
* **path:** convert to native encoding on Windows ([#806](https://github.com/rime/librime/issues/806)) ([6546689](https://github.com/rime/librime/commit/6546689)), closes [#804](https://github.com/rime/librime/issues/804) [rime/weasel#576](https://github.com/rime/weasel/issues/576) [rime/weasel#1080](https://github.com/rime/weasel/issues/1080)
* don't compress the token during collecting dict entries ([#762](https://github.com/rime/librime/issues/762)) ([#768](https://github.com/rime/librime/issues/768)) ([767ebad](https://github.com/rime/librime/commit/767ebad))


### Features

* **api:** highlight_candidate*, change_page ([142902d](https://github.com/rime/librime/commit/142902d)), closes [#620](https://github.com/rime/librime/issues/620)
* **engine:** translate zero-length prediction ([8f2e8d6](https://github.com/rime/librime/commit/8f2e8d6))
* **key_binder:** add `when: predicting` condition ([#751](https://github.com/rime/librime/issues/751)) ([3bc65c9](https://github.com/rime/librime/commit/3bc65c9))
* **rime_api:** add RimeApi::set_input ([#771](https://github.com/rime/librime/issues/771)) ([de12d6a](https://github.com/rime/librime/commit/de12d6a)), closes [#547](https://github.com/rime/librime/issues/547)
* add reload command for rime_api_console ([#741](https://github.com/rime/librime/issues/741)) ([9b2689b](https://github.com/rime/librime/commit/9b2689b))


### Performance Improvements

* less nest in filesystem iteration When CleanOldLogFiles::Run ([#801](https://github.com/rime/librime/issues/801)) ([9ec1711](https://github.com/rime/librime/commit/9ec1711))


### BREAKING CHANGES

* **path:** Most `string` filenames in APIs are changed to `path`;
`installation.yaml` should be UTF-8 encoded.

Previouly on Windows, the file can be written in local encoding to
enable paths with non-ASCII characters. It should be updated to UTF-8
after this change.

Details of the code refactor

Wrap `std::filesystem::path` in a thin wrapper class `rime::path` which calls `std::filesystem::u8path` in the constructor on Windows.

Operator `/=` and `/` are also overloaded to convert the right operand from UTF-8 string to native path.

Follow these rules to apply correct conversion between `string` and `rime::path`:

- construct `rime::path` with UTF-8 encoded string;
- get native string by `path::u8string`;
- to extract UTF-8 string from `path`, for example to find schema ID from file name, call `path::u8string`;
- avoid implicit conversion from string, which results in `std::filesystem::path` without performing UTF-8 to native conversion;
- explicitly construct `rime::path` from `std::filesystem::path` before append operation, to ensure the overloaded operator with string conversion is used.



<a name="1.9.0"></a>
# [1.9.0](https://github.com/rime/librime/compare/1.8.5...1.9.0) (2023-09-16)


### Bug Fixes

* avoid invalid page_size ([#679](https://github.com/rime/librime/issues/679)) ([53ce306](https://github.com/rime/librime/commit/53ce306))


### Features

* add `append_comment` option to `reverse_lookup_filter` ([#699](https://github.com/rime/librime/issues/699)) ([f079c3c](https://github.com/rime/librime/commit/f079c3c))
* **dict:** shorten the data struct used by Vocabulary ([210ab6c](https://github.com/rime/librime/commit/210ab6c))
* **switches:** abbreviate state labels ([45cd64b](https://github.com/rime/librime/commit/45cd64b))


### Performance Improvements

* release memory of temporary fields ([#690](https://github.com/rime/librime/issues/690)) ([43ae3a9](https://github.com/rime/librime/commit/43ae3a9))
* replace rbtree with vector of pair ([#684](https://github.com/rime/librime/issues/684)) ([c515afb](https://github.com/rime/librime/commit/c515afb))
* **dict:** improve dict complile performance ([#663](https://github.com/rime/librime/issues/663)) ([bd3c7c7](https://github.com/rime/librime/commit/bd3c7c7))



<a name="1.8.5"></a>
## [1.8.5](https://github.com/rime/librime/compare/1.8.4...1.8.5) (2023-02-05)


### Bug Fixes

* **selector:** do not match fallback modifier ([21adf97](https://github.com/rime/librime/commit/21adf97)), closes [#609](https://github.com/rime/librime/issues/609)



<a name="1.8.4"></a>
## [1.8.4](https://github.com/rime/librime/compare/1.8.3...1.8.4) (2023-01-30)



<a name="1.8.3"></a>
## [1.8.3](https://github.com/rime/librime/compare/1.8.2...1.8.3) (2023-01-25)


### Bug Fixes

* skip key combos with super modifier ([80bef8f](https://github.com/rime/librime/commit/80bef8f)), closes [#458](https://github.com/rime/librime/issues/458)



<a name="1.8.2"></a>
## [1.8.2](https://github.com/rime/librime/compare/1.8.1...1.8.2) (2023-01-25)



<a name="1.8.1"></a>
## [1.8.1](https://github.com/rime/librime/compare/1.8.0...1.8.1) (2023-01-25)



<a name="1.8.0"></a>
# [1.8.0](https://github.com/rime/librime/compare/1.7.3...1.8.0) (2023-01-25)


### Bug Fixes

* **algo, dict:** add an extra set of parenthesis to std::min ... ([f66d330](https://github.com/rime/librime/commit/f66d330))
* **charset_filter:** Update `is_extended_cjk` to compatible with Unicode 15 ([946e852](https://github.com/rime/librime/commit/946e852))
* **chord_composer:** first key missing from raw input ([33d85ad](https://github.com/rime/librime/commit/33d85ad))
* **chord_composer:** invisible placeholder char prepended to output ([0ec7a2d](https://github.com/rime/librime/commit/0ec7a2d))
* **ci:** download and extract boost ([200e3ec](https://github.com/rime/librime/commit/200e3ec))
* **ci:** install Rime plugins' dependencies ([7677dae](https://github.com/rime/librime/commit/7677dae))
* **ci:** no hardcoded boost version in ci scripts ([3ae72a3](https://github.com/rime/librime/commit/3ae72a3))
* **dict_compiler:** shrink .reverse.bin file to fit after build ([2af37e0](https://github.com/rime/librime/commit/2af37e0))
* **reverse_lookup_dictionary:** writing data to overflow buffer ([46e3066](https://github.com/rime/librime/commit/46e3066)), closes [#456](https://github.com/rime/librime/issues/456)
* **setup:** avoid glog log macros conflict with macros of Windows ([24e1612](https://github.com/rime/librime/commit/24e1612))
* fix missing control paths issue ([87e4c73](https://github.com/rime/librime/commit/87e4c73))
* qualify std::exp after including <cmath> ([bc589fd](https://github.com/rime/librime/commit/bc589fd)), closes [#462](https://github.com/rime/librime/issues/462) [#476](https://github.com/rime/librime/issues/476)
* **ci:** update ci script for windows ([0358a21](https://github.com/rime/librime/commit/0358a21))
* **dictionary:** had unsorted chunks after the previous chunk ended ([dd6af3d](https://github.com/rime/librime/commit/dd6af3d)), closes [#507](https://github.com/rime/librime/issues/507)
* **editor:** `back_syllable` should reopen selected words ([a3fdad8](https://github.com/rime/librime/commit/a3fdad8))
* replace bintry mirror with jfrog ([a1203fc](https://github.com/rime/librime/commit/a1203fc))


### Features

* **api:** get_state_label returns the state label for UI display ([8f9e9c7](https://github.com/rime/librime/commit/8f9e9c7)), closes [#447](https://github.com/rime/librime/issues/447)
* **ci:** add GitHub action ci support ([56e9d2a](https://github.com/rime/librime/commit/56e9d2a))
* **ci:** add pull request ci ([94bb5d5](https://github.com/rime/librime/commit/94bb5d5))
* **ci:** add tag release ci ([aafe5a3](https://github.com/rime/librime/commit/aafe5a3))
* **context:** allow composing with zero input ([8e2826a](https://github.com/rime/librime/commit/8e2826a))
* **key_binder:** single select a radio group option ([5e8aca2](https://github.com/rime/librime/commit/5e8aca2))
* **key_binder:** toggle the switch defined at index `toggle: '@n'` ([6ca7803](https://github.com/rime/librime/commit/6ca7803)), closes [#554](https://github.com/rime/librime/issues/554)
* **navigator,selector:** vertical navigation ([d79f6b3](https://github.com/rime/librime/commit/d79f6b3)), closes [#543](https://github.com/rime/librime/issues/543)
* **plugins:** support plugin specs in slug@branch format ([7d11455](https://github.com/rime/librime/commit/7d11455))
* **selector:** handle repeated key until release ([8d93e9f](https://github.com/rime/librime/commit/8d93e9f))
* add windows ci job ([14ee47a](https://github.com/rime/librime/commit/14ee47a))
* delete selected candidate ([#557](https://github.com/rime/librime/issues/557)) ([43023e1](https://github.com/rime/librime/commit/43023e1))
* half/full-shape labels for more characters ([f2c7e40](https://github.com/rime/librime/commit/f2c7e40)), closes [#594](https://github.com/rime/librime/issues/594)
* **simplifier:** can be configured to return empty comment ([#541](https://github.com/rime/librime/issues/541)) ([2368034](https://github.com/rime/librime/commit/2368034))



<a name="1.7.3"></a>
## [1.7.3](https://github.com/rime/librime/compare/1.7.2...1.7.3) (2021-02-11)


### Bug Fixes

* **memory:** possibly dereferencing null pointer ([1c2644d](https://github.com/rime/librime/commit/1c2644d)), closes [#441](https://github.com/rime/librime/issues/441)
* **plugins:** buffer overflow in plugin lotem/librime-octagram ([b0b435b](https://github.com/rime/librime/commit/b0b435b)), closes [rime/squirrel#512](https://github.com/rime/squirrel/issues/512)



<a name="1.7.2"></a>
## [1.7.2](https://github.com/rime/librime/compare/1.7.1...1.7.2) (2021-02-07)


### Bug Fixes

* **chord_composer:** should clear raw input after committing text ([79b34ab](https://github.com/rime/librime/commit/79b34ab))



<a name="1.7.1"></a>
## [1.7.1](https://github.com/rime/librime/compare/1.7.0...1.7.1) (2021-02-06)


### Bug Fixes

* **chord_composer:** press Return key to commit raw key sequence ([2b25861](https://github.com/rime/librime/commit/2b25861))



<a name="1.7.0"></a>
# [1.7.0](https://github.com/rime/librime/compare/1.6.1...1.7.0) (2021-01-17)


### Bug Fixes

* **chord_composer:** more safely handle the placeholder ZWSP ([025d9fb](https://github.com/rime/librime/commit/025d9fb))
* **cmake:** use full paths defined by GNUInstallDirs ([bb8c263](https://github.com/rime/librime/commit/bb8c263)), closes [#424](https://github.com/rime/librime/issues/424)
* **opencc:** update submodule to fix [#425](https://github.com/rime/librime/issues/425) ([3fa1571](https://github.com/rime/librime/commit/3fa1571))
* **script_translator:** always_show_comments also applies to phrases ([440a97c](https://github.com/rime/librime/commit/440a97c)), closes [#272](https://github.com/rime/librime/issues/272) [#419](https://github.com/rime/librime/issues/419)
* **table_translator:** index out of bound access in string ([ff7acdc](https://github.com/rime/librime/commit/ff7acdc))


### Features

* **chareset_filter:** add CJK Compatibility Ideographs in is_extended_cjk() ([3cb1128](https://github.com/rime/librime/commit/3cb1128)), closes [#305](https://github.com/rime/librime/issues/305)
* **setup:** find and load external RIME plugins as shared libs [#431](https://github.com/rime/librime/issues/431) ([b2abd09](https://github.com/rime/librime/commit/b2abd09))



<a name="1.6.1"></a>
## [1.6.1](https://github.com/rime/librime/compare/1.6.0...1.6.1) (2020-09-21)


### Bug Fixes

* **rime_api.cc:** dangling pointer returned from RimeGetSharedDataDir ([78abaa8](https://github.com/rime/librime/commit/78abaa8))



<a name="1.6.0"></a>
# [1.6.0](https://github.com/rime/librime/compare/1.5.3...1.6.0) (2020-09-20)


### Bug Fixes

* **ascii_composer:** do not consume Shift key release ([debc2c0](https://github.com/rime/librime/commit/debc2c0))
* **ascii_composer:** first read ascii_composer/good_old_caps_lock from schema config ([3fc56c4](https://github.com/rime/librime/commit/3fc56c4))
* **chord_composer:** commit raw input with uppercase letters ([cc983d5](https://github.com/rime/librime/commit/cc983d5))
* **CMakeLists.txt:** ensure paths in pkgconfig file are absolute ([0e96e51](https://github.com/rime/librime/commit/0e96e51))
* **CMakeLists.txt:** would not use signals v1 due to a typo ([6662a28](https://github.com/rime/librime/commit/6662a28)), closes [#225](https://github.com/rime/librime/issues/225)
* **custom_settings:** accept "*.schema" as config id ([604da0b](https://github.com/rime/librime/commit/604da0b))
* **dict:** issues with user db recovery ([0f3d0df](https://github.com/rime/librime/commit/0f3d0df))
* **dict_compiler:** build prism with loaded syllabary when not rebuilding primary table ([93fe827](https://github.com/rime/librime/commit/93fe827))
* **plugins/CMakeLists.txt:** avoid rime_library linking to itself via rime_plugins_deps ([fe744db](https://github.com/rime/librime/commit/fe744db))
* **rime_api.cc:** check struct has member of non-pointer type ([090dfa4](https://github.com/rime/librime/commit/090dfa4))
* **rime_api.cc:** using unchecked fields introduced an ABI breakage ([62bbead](https://github.com/rime/librime/commit/62bbead)), closes [/github.com/rime/librime/pull/328#pullrequestreview-335125464](https://github.com//github.com/rime/librime/pull/328/issues/pullrequestreview-335125464)
* **rime_test:** set data directories to working directory using rime::SetupDeployer API ([7c08a90](https://github.com/rime/librime/commit/7c08a90))
* **simplifier:** opencc::DictEntry::Values() type change in opencc 1.1.0 ([beae5b1](https://github.com/rime/librime/commit/beae5b1)), closes [#367](https://github.com/rime/librime/issues/367)
* **user_db:** pointer cast error caused by multiple inheritance ([2ed780b](https://github.com/rime/librime/commit/2ed780b))
* use official emoji 12.0 data ([#304](https://github.com/rime/librime/issues/304)) ([75a60dc](https://github.com/rime/librime/commit/75a60dc))


### Features

* **api:** implement capnproto api ([873f648](https://github.com/rime/librime/commit/873f648))
* **api:** include candidate labels in proto message ([aae7a0c](https://github.com/rime/librime/commit/aae7a0c))
* **charset_filter:** support charset options with emoji ([#293](https://github.com/rime/librime/issues/293)) ([943c95b](https://github.com/rime/librime/commit/943c95b))
* **charset_filter:** support CJK Unified Ideographs Extension G ([#393](https://github.com/rime/librime/issues/393)) ([0a1573d](https://github.com/rime/librime/commit/0a1573d))
* **chord_composer:** support chording with Shift keys ([94cf479](https://github.com/rime/librime/commit/94cf479))
* **chord_composer:** use Control, Alt, Shift to input chord ([f3a2ad0](https://github.com/rime/librime/commit/f3a2ad0))
* **dictionary:** packs extends the dictionary with extra binary table files ([930074c](https://github.com/rime/librime/commit/930074c))
* **key_binder:** bind key to a key sequence ([3b5dbf6](https://github.com/rime/librime/commit/3b5dbf6)), closes [#301](https://github.com/rime/librime/issues/301)
* **logging:** setup min log level, log dir and set file mode to log files ([90839b0](https://github.com/rime/librime/commit/90839b0))
* **selector:** support 4 combinations of horizontal/vertical text orientation and stacked/linear candidate list layout ([c498f71](https://github.com/rime/librime/commit/c498f71))
* **selector:** support vertical UI ([dbb35c6](https://github.com/rime/librime/commit/dbb35c6))
* **switcher:** enable schema in cases where conditions are met ([217c72b](https://github.com/rime/librime/commit/217c72b))
* **tools/rime_proto_console:** demo for proto api ([d88ef9f](https://github.com/rime/librime/commit/d88ef9f))


### Performance Improvements

* **poet:** optimize for performance in making sentences (~40% faster) ([0853465](https://github.com/rime/librime/commit/0853465))



<a name="1.5.3"></a>
## [1.5.3](https://github.com/rime/librime/compare/1.5.2...1.5.3) (2019-06-22)


### Bug Fixes

* **cmake, xcode.mk:** find optional dependency icu, while building xcode/release-with-icu target  [skip appveyor] ([17a80f8](https://github.com/rime/librime/commit/17a80f8))
* **single_char_filter:** broken in librime 1.5.2 ([6948a62](https://github.com/rime/librime/commit/6948a62))


### Features

* **appveyor:** build variant "rime-with-plugins" for tagged commits  [skip travis] ([eef8c30](https://github.com/rime/librime/commit/eef8c30))
* **travis-ci:** build variant "rime-with-plugins" for tagged commits  [skip appveyor] ([cf11c27](https://github.com/rime/librime/commit/cf11c27))
* **travis-ci:** deploy artifacts for macOS to GitHub releases  [skip appveyor] ([3f03784](https://github.com/rime/librime/commit/3f03784))



<a name="1.5.2"></a>
## [1.5.2](https://github.com/rime/librime/compare/1.5.1...1.5.2) (2019-06-17)


### Bug Fixes

* **user_dictionary, contextual_translation:** fix user phrase quality; order contextual suggestions by type ([69d5c32](https://github.com/rime/librime/commit/69d5c32))



<a name="1.5.1"></a>
## [1.5.1](https://github.com/rime/librime/compare/1.5.0...1.5.1) (2019-06-16)


### Bug Fixes

* **user_dictionary:** make user phrases comparable in weight to system words ([982f69d](https://github.com/rime/librime/commit/982f69d))



<a name="1.5.0"></a>
# [1.5.0](https://github.com/rime/librime/compare/1.4.0...1.5.0) (2019-06-06)


### Bug Fixes

* **ci:** update build script ([84a1a1b](https://github.com/rime/librime/commit/84a1a1b))
* **ci:** use submodules in AppVeyor CI build script ([7b515b4](https://github.com/rime/librime/commit/7b515b4))
* **cmake:** libboost Windows XP compatibility fix ([#270](https://github.com/rime/librime/issues/270)) ([fecfe39](https://github.com/rime/librime/commit/fecfe39)), closes [rime/weasel#337](https://github.com/rime/weasel/issues/337)
* **CMakeLists.txt:** install header files in all platforms ([821d563](https://github.com/rime/librime/commit/821d563))
* **CMakeLists.txt:** set "-std=c++11" in CMAKE_CXX_FLAGS ([5d8a836](https://github.com/rime/librime/commit/5d8a836))
* **config/plugins.h:** memory leak caused by non-virtual destructor ([316a659](https://github.com/rime/librime/commit/316a659)), closes [#259](https://github.com/rime/librime/issues/259)
* **deploy:** treat schema dependencies as optional; do not report errors if missing ([ff3d5e9](https://github.com/rime/librime/commit/ff3d5e9))
* **engine:** schema doesn't match the one used by switcher ([e41bb63](https://github.com/rime/librime/commit/e41bb63)), closes [#269](https://github.com/rime/librime/issues/269)
* **rime_levers_api.h:** customize_bool() misused `bool` type ([42bacc5](https://github.com/rime/librime/commit/42bacc5))
* **syllabifier:** enable_completion not working ([2714131](https://github.com/rime/librime/commit/2714131)), closes [#343](https://github.com/rime/librime/issues/343)
* **table_translator:** null pointer exception when dict entries are filtered ([77438a9](https://github.com/rime/librime/commit/77438a9))
* **test:** compile error in unit test ([7076d9e](https://github.com/rime/librime/commit/7076d9e))
* **travis-install.sh:** working directory ([97220ce](https://github.com/rime/librime/commit/97220ce))


### Features

* **appveyor:** install RIME_PLUGINS  [skip travis] ([c7ce66f](https://github.com/rime/librime/commit/c7ce66f))
* **CMakeList.txt:** add plugin build support ([#257](https://github.com/rime/librime/issues/257)) ([dfa341b](https://github.com/rime/librime/commit/dfa341b))
* **contextual_translation:** weight and re-order phrases by context ([2390da3](https://github.com/rime/librime/commit/2390da3))
* **dict:** specify vocabulary db name in dict settings ([dcdc301](https://github.com/rime/librime/commit/dcdc301))
* **grammar:** compare homophones/homographs in sentence ([9248a6b](https://github.com/rime/librime/commit/9248a6b))
* **install-plugins.sh:** git-clone or update plugins ([70483b4](https://github.com/rime/librime/commit/70483b4))
* **poet:** find best sentence candidates ([b3f4005](https://github.com/rime/librime/commit/b3f4005))
* **rime_api:** get candidate list from index ([c587900](https://github.com/rime/librime/commit/c587900))
* **translator:** contextual suggestions in partially selected sentence ([12a7501](https://github.com/rime/librime/commit/12a7501))
* **translator:** look at preceding text when making sentence ([6ae34de](https://github.com/rime/librime/commit/6ae34de))
* **travis-ci:** install plugins specified in envvar RIME_PLUGINS ([c857639](https://github.com/rime/librime/commit/c857639))


### Performance Improvements

* **dictionary:** refactor DictEntryIterator and do partial sort ([0258c7f](https://github.com/rime/librime/commit/0258c7f))


### BREAKING CHANGES

* **rime_levers_api.h:** in signature of C API function `customize_bool()`,
change type `bool` to `Bool` (alias of `int`).

Impact: the changed function is not in use by any first party code,
known to be in use by osfans/trime.



<a name="1.4.0"></a>
# [1.4.0](https://github.com/rime/librime/compare/1.3.2...1.4.0) (2019-01-16)


### Bug Fixes

* **config:** user_config should not fall back to shared data ([68c8a34](https://github.com/rime/librime/commit/68c8a34)), closes [#271](https://github.com/rime/librime/issues/271)
* **SymlinkingPrebuiltDictionaries:** remove dangling symlinks ([5ad333d](https://github.com/rime/librime/commit/5ad333d)), closes [#241](https://github.com/rime/librime/issues/241)
* **SymlinkingPrebuiltDictionaries:** remove dangling symlinks ([f8e4ebf](https://github.com/rime/librime/commit/f8e4ebf)), closes [#241](https://github.com/rime/librime/issues/241)


### Features

* spelling correction ([#228](https://github.com/rime/librime/issues/228)) ([ad3638a](https://github.com/rime/librime/commit/ad3638a))
* **Dockerfile:** for build ([#246](https://github.com/rime/librime/issues/246)) ([cafd0d5](https://github.com/rime/librime/commit/cafd0d5))



## [1.3.2](https://github.com/rime/librime/compare/1.3.1...1.3.2) (2018-11-12)


### Bug Fixes

* **CMakeLists.txt:** do not link binaries when building static library ([99573e3](https://github.com/rime/librime/commit/99573e3))
* **CMakeLists.txt:** do not require boost::signals, which will be deprecated in Boost 1.69 ([8a9ef3b](https://github.com/rime/librime/commit/8a9ef3b)), closes [#225](https://github.com/rime/librime/issues/225)
* **config_compiler:** ambiguous operator overload with cmake option ENABLE_LOGGING=OFF ([b86b647](https://github.com/rime/librime/commit/b86b647)), closes [#211](https://github.com/rime/librime/issues/211)
* **config_compiler:** support creating list in-place by __patch and __merge ([0784eb0](https://github.com/rime/librime/commit/0784eb0))
* **table_translator:** enable encoding uniquified commit history ([74e31bc](https://github.com/rime/librime/commit/74e31bc))


### Features

* **language:** shared user dictionary per language (Closes [#184](https://github.com/rime/librime/issues/184)) ([#214](https://github.com/rime/librime/issues/214)) ([9f774e7](https://github.com/rime/librime/commit/9f774e7))
* always_show_comments option ([#220](https://github.com/rime/librime/issues/220)) ([19cea07](https://github.com/rime/librime/commit/19cea07))



<a name="1.3.1"></a>
## [1.3.1](https://github.com/rime/librime/compare/1.3.0...1.3.1) (2018-04-01)


### Bug Fixes

* **config_file_update:** clean up deprecated user copy ([#193](https://github.com/rime/librime/issues/193)) ([8d8d2e6](https://github.com/rime/librime/commit/8d8d2e6))
* **thirdparty/src/leveldb:** do not link to snappy library ([6f6056a](https://github.com/rime/librime/commit/6f6056a))



<a name="1.3.0"></a>
# 1.3.0 (2018-03-09)


### Bug Fixes

* **CMakeLists.txt, build.bat:** install header files (public API) ([06c9e86](https://github.com/rime/librime/commit/06c9e86))
* **config_compiler:** "/" mistaken as path separator in merged map key ([#192](https://github.com/rime/librime/issues/192)) ([831ffba](https://github.com/rime/librime/commit/831ffba)), closes [#190](https://github.com/rime/librime/issues/190)
* **ConfigFileUpdate:** no need to create user build if shared build is up-to-date ([cafd5c4](https://github.com/rime/librime/commit/cafd5c4))
* **SchemaUpdate:** read compiled schema from shared build if there is no user build ([45a04dd](https://github.com/rime/librime/commit/45a04dd))
* **simplifier:** fix typo ([9e1114e](https://github.com/rime/librime/commit/9e1114e)), closes [#183](https://github.com/rime/librime/issues/183)
* **user_db:** unwanted implicit instantiation of UserDbFormat template ([3cbc9cb](https://github.com/rime/librime/commit/3cbc9cb)), closes [#188](https://github.com/rime/librime/issues/188)


### Chores

* **release tag:** deprecating tag name prefix 'rime-' in favor of semver 'X.Y.Z'


### BREAKING CHANGES

* **release tag:** After 1.3.0 release, we'll no longer be creating tags in the format 'rime-X.Y.Z'. Downstream packagers please change automated scripts accordingly.



<a name="1.2.10"></a>
## 1.2.10 (2018-02-21)


### Bug Fixes

* **config_compiler:** linking failure on blocking root node of a dependency resource ([ecf3397](https://github.com/rime/librime/commit/ecf3397))
* table_translator not making sentence if table entry is hidden by charset filter. ([77eb12e](https://github.com/rime/librime/commit/77eb12e))
* **appveyor.install.bat:** switch to a more stable download server for libboost ([bcc4d10](https://github.com/rime/librime/commit/bcc4d10))
* **appveyor.yml:** archive header files ([c8b1e67](https://github.com/rime/librime/commit/c8b1e67))
* **ascii_composer:** support key binding Shift+space in ascii mode ([7077389](https://github.com/rime/librime/commit/7077389))
* **build.bat:** fix build errors with VS2015 build tools ([ec940c6](https://github.com/rime/librime/commit/ec940c6))
* **calculus, recognizer:** memory leak due to unchecked regex error ([19ddc1e](https://github.com/rime/librime/commit/19ddc1e)), closes [#171](https://github.com/rime/librime/issues/171)
* **chord_composer:** allow editor to define BackSpace key behavior ([7f41f65](https://github.com/rime/librime/commit/7f41f65))
* **chord_composer:** letters with modifier keys should not be committed by a following enter key ([aab5eb8](https://github.com/rime/librime/commit/aab5eb8))
* **ci:** call cmake under /usr/local with sudo by passing $PATH environment variable ([a0e6d2f](https://github.com/rime/librime/commit/a0e6d2f))
* **cmake:** fix build break for mingw ([939893c](https://github.com/rime/librime/commit/939893c))
* **config:** auto save modified config data; fixes [#144](https://github.com/rime/librime/issues/144) ([2736f4b](https://github.com/rime/librime/commit/2736f4b))
* **config:** treat "@" as map key rather than list index ([a1df9c5](https://github.com/rime/librime/commit/a1df9c5))
* **config_compiler:** duplicate PendingChild dependencies happen from multiple commands on the same node ([25c28f8](https://github.com/rime/librime/commit/25c28f8))
* **config_compiler:** enforce dependency priorities ([69a6f3e](https://github.com/rime/librime/commit/69a6f3e))
* **config_compiler:** null value should not overwrite a normal key in a merged tree ([4ecae44](https://github.com/rime/librime/commit/4ecae44))
* **config_compiler:** template operator overload had compile error with NDK ([71817a0](https://github.com/rime/librime/commit/71817a0))
* **config/build_info_plugin:** referenced but unavailable resources should also be recorded ([cd46f7a](https://github.com/rime/librime/commit/cd46f7a))
* **ConfigFileUpdate:** should succeed if shared copy does not exist ([8a3e25c](https://github.com/rime/librime/commit/8a3e25c))
* **custom_settings:** fall back to $shared_data_dir/build when loading config ([caf8ebb](https://github.com/rime/librime/commit/caf8ebb))
* **custom_settings:** load built settings from $user_data_dir/build directory ([463dc09](https://github.com/rime/librime/commit/463dc09))
* **deployment_tasks:** symbols.yaml is no longer a build target ([f920e4f](https://github.com/rime/librime/commit/f920e4f))
* **dict_compiler:** prism should load compiled schema ([c2fd0cf](https://github.com/rime/librime/commit/c2fd0cf)), closes [#176](https://github.com/rime/librime/issues/176)
* **key_event:** KeySequence::repr() prefer unescaped punctuation characters ([aa43e5e](https://github.com/rime/librime/commit/aa43e5e))
* **levers:** update deployment tasks for copy-free resource resolution ([1f86413](https://github.com/rime/librime/commit/1f86413))
* **Makefile:** make install-debug; do return error code on mac ([1177142](https://github.com/rime/librime/commit/1177142))
* **rime_api:** use user_config_open() to access user.yaml ([4e4a491](https://github.com/rime/librime/commit/4e4a491))
* **rime_console:** not showing switcher's context ([632cf4b](https://github.com/rime/librime/commit/632cf4b))
* **schema:** create a "schema" component that opens Config by schema_id ([555f990](https://github.com/rime/librime/commit/555f990))
* **simplifier:** fix crash if no opencc file ([091cb9d](https://github.com/rime/librime/commit/091cb9d))
* **simplifier:** tips option for show_in_comment simplifier ([e7bb757](https://github.com/rime/librime/commit/e7bb757))
* **uniquifier:** half of the duplicate candidates remain after dedup [Closes [#114](https://github.com/rime/librime/issues/114)] ([2ab76bc](https://github.com/rime/librime/commit/2ab76bc))


### Features

* **build.bat:** customize build settings via environment variables ([#178](https://github.com/rime/librime/issues/178)) ([1678b75](https://github.com/rime/librime/commit/1678b75))
* **chord_composer:** accept escaped chording keys ([79a32b2](https://github.com/rime/librime/commit/79a32b2))
* **chord_composer:** support chording with function keys ([48424d3](https://github.com/rime/librime/commit/48424d3))
* **config:** add config compiler plugin that includes default:/menu into schema ([b51dda8](https://github.com/rime/librime/commit/b51dda8))
* **config:** best effort resolution for circurlar dependencies ([2e52d54](https://github.com/rime/librime/commit/2e52d54))
* **config:** build config files if source files changed ([0d79712](https://github.com/rime/librime/commit/0d79712))
* **config:** config compiler plugins that port legacy features to the new YAML syntax ([a7d253e](https://github.com/rime/librime/commit/a7d253e))
* **config:** config_builder saves output to $rime_user_dir/build/ ([e596155](https://github.com/rime/librime/commit/e596155))
* **config:** references to optional config resources, ending with "?" ([14ec858](https://github.com/rime/librime/commit/14ec858))
* **config:** save __build_info in compiled config ([45a7337](https://github.com/rime/librime/commit/45a7337))
* **config:** separate out config_builder and user_config components ([9e9493b](https://github.com/rime/librime/commit/9e9493b))
* **config:** support append and merge syntax ([04dcf42](https://github.com/rime/librime/commit/04dcf42))
* **customizer:** disable saving patched config files ([88f5a0c](https://github.com/rime/librime/commit/88f5a0c))
* **detect_modifications:** quick test based on last write time of files ([285fbcc](https://github.com/rime/librime/commit/285fbcc))
* **dict:** no conditional compilation on arm ([85b945f](https://github.com/rime/librime/commit/85b945f))
* **dict:** relocate binary files to $user_data_dir/build ([bc66a47](https://github.com/rime/librime/commit/bc66a47))
* **dict:** use resource resolver to find dictionary files ([8ea08b3](https://github.com/rime/librime/commit/8ea08b3))
* add property notifier ([fa7b5a5](https://github.com/rime/librime/commit/fa7b5a5))
* **resource_resolver:** add class and unit test ([03ee8b4](https://github.com/rime/librime/commit/03ee8b4))
* **resource_resolver:** fallback root path ([02151da](https://github.com/rime/librime/commit/02151da))
* **translator:** add history_translator ([#115](https://github.com/rime/librime/issues/115)) ([ae13354](https://github.com/rime/librime/commit/ae13354))



<a name="1.2.9"></a>
## 1.2.9 (2014-12-14)

* **rime_api.h:** add `RIME_MODULE_LIST`, `RIME_REGISTER_MODULE_GROUP`.
* **Makefile:** add make targets `thirdparty/*` to build individual libraries.
* **legacy/src/legacy_module.cc:** plugin module `rime-legacy` for GPL code,
  providing component `legacy_userdb` for user dictionary upgrade.
* **src/setup.cc:** define module groups `"default"` and `"deployer"`, to avoid
	naming a list of built-bin modules in `RimeTraits::modules`.
* **test/table_test.cc:** fix random segment faults when run shuffled.
* **thirdparty/src/leveldb:** new dependency LevelDB, replacing Kyoto Cabinet.
* **dict/level_db:** userdb implementation based on LevelDB, replacing treeDb.
* **dict/tree_db:** moved to `legacy/src/`.
* **dict/user_db:** refactored and modularized to ease adding implementations.
* **gear/cjk_minifier:** support CJK Extension E.
* **gear/memory:** save cached phrases as soon as the next composition begins.
* **gear/recognizer:** match space iff set `recognizer/use_space: true`.
* **gear/simplifier:** catch and log OpenCC exceptions when loading.
* **gear/single_char_filter:** bring single character candidates to the front.
* **gear/simplifier:** adapt to OpenCC 1.0 API.
* **thirdparty/src/opencc:** update OpenCC to v1.0.2 (incompatible with v0.4).
* **lever/deployment_tasks:** update and rename task `user_dict_upgrade`.



<a name="1.2"></a>
## 1.2 (2014-07-15)

* **rime_api:** add API functions to access complex structures in config;
  add API to get the raw input and cursor position, or to select a candidate.
* **config:** support references to list elements in key paths.
  eg. `schema_list/@0/schema` is the id of the first schema in schema list.
* **switcher:** enable folding IME options in the switcher menu.
* **dict_compiler:** also detect changes in essay when updating a dictionary;
  support updating prism without the source file of the dictionary.
* **preset_vocabulary:** load `essay.txt` instead of `essay.kct`.
* **reverse_lookup_dictionary:** adopt a new file format with 50% space saving.
* **table:** add support for a new binary format with 20% space saving;
  fix alignment on ARM.
* **ascii_composer:** do not toggle IME states when long pressing `Shift` key;
  support discarding unfinished input when switching to ASCII mode.
* **affix_segmentor:** fix issues with selecting a partial-match candidate.
* **chord_composer:** commit raw input composed with original key strokes.
* **cjk_minifier:** a filter to hide characters in CJK extension set, works
  with `script_translator`.
* **navigator:** do not use `BackSpace` to revert selecting a candidate but to
  edit the input after moving the cursor left or right.
* **punctuator:** support `ascii_punct` option for switching between Chinese and
  Western (ASCII) punctuations.
* **speller:** auto-select candidates by pattern matching against the code;
  fix issues to cooperate with punctuator.
* **CMakeLists.txt:** add options `ENABLE_LOGGING` and `BOOST_USE_CXX11`;
  introduce a new dependency: `libmarisa`.
* **cmake/FindYamlCpp.cmake:** check the availability of the new (v0.5) API.
* **sample:** the directory containing a sample plug-in module.
* **tools/rime_patch.cc:** a command line tool to create patches.
* **thirdparty:** include source code of third-party libraries to ease
  building librime on Windows and Mac.



<a name="1.1"></a>
## 1.1 (2013-12-26)

* **new build dependency:** compiler with C++11 support.
  tested with GCC 4.8.2, Apple LLVM version 5.0, MSVC 12 (2013).
* **encoder:** disable warnings for phrase encode failures in log output;
  limit the number of results in encoding a phrase with multiple solutions.
* **punctuator:** fixed a bug in matching nested "pairs of 'symbols'".
* **speller:** better support for auto-committing, allowing users of table
  based input schema to omit explicitly selecting candidates in many cases.
* **schema_list_translator:** option for static schema list order.
* **table_translator:** fixed the range of CJK-D in charset filter.



<a name="1.0"></a>
## 1.0 (2013-11-10)

* **rime_api:** version 1.0 breaks ABI compatiblility.

  the minimum changes in code required to migrate from rime 0.9 api is to
  initialize `RimeTraits` with either `RIME_STRUCT` or `RIME_STRUCT_INIT` macro.

  while source code compatibility is largely maintained with the exception
  of the aforementioned `RimeTraits` structure, rime 1.0 introduces a version
  controlled `RimeApi` structure which provides all the api functions.
* **module:** support adding modules; modularize `gears` and `levers`.
* **ticket:** used to instantiate components and to associate the instance with
  a name space in the configuration.
* **encoder:** encode new phrases for `table_translator` and `script_translator`
  using different rules.
* **affix_segmentor:** strip optional prefix and suffix from a code segment.
* **reverse_lookup_filter:** lookup candidate text for code in a specified
  dictionary.
* **shape:** add full-shape support.
* **key_binder:** switch input schemata and toggle options with hotkeys.
* **switcher:** list input schemata ordered by recency; support radio options.
* **tsv:** fix reading user dict snapshot files with DOS line endings.
* **entry_collector:** support custom order of table columns in `*.dict.yaml`.
* **CMakeLists.txt:** add options `BUILD_TEST` and `BUILD_SEPARATE_LIBS`.



<a name="0.9.9"></a>
## 0.9.9 (2013-05-05)

* **config:** update yaml-cpp to version 0.5 (with new API); emit prettier yaml.
* **deployer:** introduce a work thread for ordinary background tasks.
* **algo/calculus:** `fuzz` calculation, to create lower quality spellings.
* **dict/dict_compiler:** importing external table files into `*.dict.yaml`.
* **dict/entry_collector:** support `# no comment` directive in `*.dict.yaml`.
* **dict/table_db:** `tabledb` and `stabledb` to support custom phrase.
* **dict/user_db:** implement `plain_userdb`, in plain text files.
* **dict/user_dictionary:** recover damaged userdb in work thread.
* **gear/ascii_composer:** fix unexpected mode switching with Caps Lock.
* **gear/editor:** delete previous syllable with `Control+BackSpace`.
* **gear/*_translator:** support multiple translator instances in a engine.
* **gear/script_translator:** rename `r10n_translator` to `script_translator`.
* **lever/user_dict_manager:** create snapshots in plain userdb format.
* **rime_deployer:** with command line option `--compile`,
  dump table/prism contents into text files while compiling a dictionary.



<a name="0.9.8"></a>
## 0.9.8 (2013-02-02)

* **ascii_composer:** support customizing Caps Lock behavior.
* **speller:** support auto-selecting unique candidates.
  add options `speller/use_space` and `speller/finals` for bopomofo.
* **punctuator:** display half-shape, full-shape labels.
  support committing a phrase with a trailing space character.
  support inputting special characters with mnemonics such as `/ts`.
* **user_dictionary:** fix abnormal records introduced by a bug in merging.
* **prism, table:** avoid creating / loading incomplete dictionary files.
* **context:** clear transient options (whose names start with `_`) and
  properties when loading a different schema.
  `chord_composer` sets `_chord_typing` so that the input method program would
  know that a chord-typing schema is in use.
* **deployment_tasks.cc(BackupConfigFiles::Run):** while syncing user data,
  backup user created / modified YAML files.
* **deployer.cc(Deployer::JoinMaintenanceThread):** fix a boost-related crash.



<a name="0.9.7"></a>
## 0.9.7 (2013-01-16)

* **ascii_composer:** support changing conversion mode with Caps Lock.
  fixed Control + letter key in temporary ascii mode.
  pressing Command/Super + Shift shouldn't toggle ascii mode.
* **user_dictionary(UserDictionary::FetchTickCount):**
  tick was reset to zero when I/O error is encountered,
  messing up order of user dict entries.
* **user_dict_manager(UserDictManager::Restore):**
  used to favor imported entries too much while merging snapshots.



<a name="0.9.6"></a>
## 0.9.6 (2013-01-12)

* **rime_deployer:** manipulate user's schema list with command line options
  `--add-schema`, `--set-active-schema`
* **rime_dict_manager:** add command line option `--sync`
* **rime_api.h (RimeSyncUserData):**
  add API function to start a data syncing task in maintenance thread.
* **rime_api.h (RimeSetNotificationHandler):**
  setup a callback function to receive notifications from librime.
* **rime_api.h (RimeGetProperty, RimeSetProperty):**
  add API functions to access session specific string properties.
* **config:** support subscript, assignment operators and simplified value accessors.
* **user_db:** optimize `user_db` for space efficiency;
  avoid blocking user input when the database file needs repair.
* **user_dictionary:** add transaction support.
* **memory:** cancel memorizing newly committed phrases that has been
  immediately erased with `BackSpace` key.
* **navigator:** move caret left by syllable in phonetic input schemas.
* **express_editor:** fix problem memorizing phrases committed with return key.
* **table_translator:** add option `translator/enable_sentence`.
* **reverse_lookup_translator:**
  a reverse lookup segment can be suffixed by a delimiter.
  phonetic abbreviations now come after completion results in a mixed input scenario.



<a name="0.9.4-1"></a>
## 0.9.4-1 (2012-09-26)

* **new dependency:** 'google-glog'.
* **CMakeLists.txt:** fix x64 build.



<a name="0.9.3"></a>
## 0.9.3 (2012-09-25)

* **table_translator:** add user dictionary.
* **deployment_tasks:** automatically build schema dependencies.
* **logging:** adopt google-glog.
* **brise:** install data files from a separate package.
* **new API:** accessing schema list.
* **new API:** enabling/disabling soft cursor in preedit string.



<a name="0.9.2-1"></a>
## 0.9.2-1 (2012-07-08)

* **chord_composer:** combine multiple keys to compose a syllable at once.
* **configuration:** global `page_size` setting.
* **API:** extend the API to support inline mode.
* **table_translator:** add option to filter candidates by character set.
* **user_dictionary:** automatic recovery for corrupted databases.
* **user_dictionary:** fixed a bug that was responsible for missing user phrases.
* **rime_deployer:** a utility program to prepare Rime's workspace.
* **rime_dict_manager:** a utility program to import/export user dictionaries.
* **librime:** include `brise`, a collection of preset schemata in the package.
* **new schema:** Middle Chinese Phonetic Transcription.
* **new schema:** IPA input method in X-SAMPA.



<a name="0.9.1-1"></a>
## 0.9.1-1 (2012-05-06)

* Revised API.
