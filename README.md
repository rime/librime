<meta charset="UTF-8">

RIME: Rime Input Method Engine
===
[![Build Status](https://travis-ci.org/rime/librime.svg)](https://travis-ci.org/rime/librime)
[![Build status](https://ci.appveyor.com/api/projects/status/github/rime/librime?svg=true)](https://ci.appveyor.com/project/rime/librime)
[![GitHub release](https://img.shields.io/github/release/rime/librime.svg)](https://github.com/rime/librime/releases)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Rime with your keystrokes.

Project home
---
[rime.im](https://rime.im)

License
---
[The 3-Clause BSD License](https://opensource.org/licenses/BSD-3-Clause)

Features
===
  - A modular, extensible input method engine in cross-platform C++ code,
    built on top of open-source technologies
  - Covering features found in a large variety of Chinese input methods,
    either shape-based or phonetic-based
  - Built with native support for Traditional Chinese, conversion to Simplified
    Chinese and other regional standards via OpenCC
  - Rime input schema, a DSL in YAML syntax for fast trying out innovative ideas
    of input method design
  - Spelling Algebra, a mechanism to create variant spelling, especially useful
    for Chinese dialects
  - Support for chord-typing with a generic Qwerty keyboard

Install
===
Follow the instructions to build librime on platforms other than Linux:
  - [macOS](https://github.com/rime/librime/tree/master/README-mac.md)
  - [Windows](https://github.com/rime/librime/tree/master/README-windows.md)

Build dependencies
---
  - compiler with C++11 support
  - cmake>=2.8
  - libboost>=1.48
  - libglog (optional)
  - libleveldb
  - libmarisa
  - libopencc>=1.0.2
  - libyaml-cpp>=0.5
  - libgtest (optional)

Runtime dependencies
---
  - libboost
  - libglog (optional)
  - libleveldb
  - libmarisa
  - libopencc
  - libyaml-cpp

Build and install librime on Linux
---
```
make
sudo make install
```

Frontends
===

Official:
  - [ibus-rime](https://github.com/rime/ibus-rime): IBus frontend for Linux
  - [Squirrel](https://github.com/rime/squirrel): frontend for macOS
  - [Weasel](https://github.com/rime/weasel): frontend for Windows

Third-party:
  - [fcitx-rime](https://github.com/fcitx/fcitx-rime): Fcitx frontend for Linux
  - [iRime](https://github.com/jimmy54/iRime): frontend for iOS
  - [PIME](https://github.com/EasyIME/PIME): frontend for Windows
  - [Trime](https://github.com/osfans/trime): frontend for Android
  - [XIME](https://github.com/stackia/XIME): frontend for macOS

Plugins
===
  - [librime-legacy](https://github.com/rime/librime-legacy) Legacy module with
    GPL-licensed code

Related works
===
  - [plum](https://github.com/rime/plum): Rime configuration manager and input
    schema repository
  - [Combo Pinyin](https://github.com/rime/home/wiki/ComboPinyin): an innovative
    chord-typing practice to input Pinyin
  - essay: the vocabulary and language model for Rime
  - [SCU](https://github.com/neolee/SCU): Squirrel Configuration Utilities

Credits
===
We are grateful to the makers of the following open source libraries:

  - [Boost C++ Libraries](http://www.boost.org/) (Boost Software License)
  - [google-glog](https://github.com/google/glog) (The 3-Clause BSD License)
  - [Google Test](https://github.com/google/googletest) (The 3-Clause BSD License)
  - [LevelDB](https://github.com/google/leveldb) (The 3-Clause BSD License)
  - [marisa-trie](https://github.com/s-yata/marisa-trie) (BSD 2-Clause License, LGPL 2.1)
  - [OpenCC](https://github.com/BYVoid/OpenCC) (Apache License 2.0)
  - [yaml-cpp](https://github.com/jbeder/yaml-cpp) (MIT License)

Contributors
===
  - [佛振](https://github.com/lotem)
  - [鄒旭](https://github.com/zouxu09)
  - [Weng Xuetian](http://csslayer.info)
  - [Chongyu Zhu](http://lembacon.com)
  - [Zhiwei Liu](https://github.com/liuzhiwei)
  - [BYVoid](http://www.byvoid.com)
  - [雪齋](https://github.com/LEOYoon-Tsaw)
  - [瑾昀](https://github.com/kunki)
  - [osfans](https://github.com/osfans)
  - [jakwings](https://github.com/jakwings)
  - [Prcuvu](https://github.com/Prcuvu)
  - [hchunhui](https://github.com/hchunhui)
