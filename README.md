<meta charset="UTF-8">

RIME: Rime Input Method Engine
===
rimes with your keystrokes.

Project home
---
[rime.github.io](http://rime.github.io)

License
---
The BSD License

Features
===
  - A modular, extensible input method engine in cross-platform C++ code, built on top of open-source technologies
  - Covering features found in a large variety of Chinese input methods, either shape-based or phonetic-based
  - Built with native support for Traditional Chinese, conversion to Simplified Chinese and other regional standards via OpenCC
  - Rime input schema, a DSL in YAML syntax for fast trying out innovative ideas of input method design
  - Spelling Algebra, a mechanism to create variant spelling, especially useful for Chinese dialects
  - Support for chord-typing with a generic Qwerty keyboard

Install
===
Follow the instructions to build librime on platforms other than Linux:
  - [Mac OS X](https://github.com/rime/librime/tree/develop/README-mac.md)
  - [Windows](https://github.com/rime/librime/tree/develop/README-windows.md)

Build dependencies
---
  - compiler with C++11 support
  - cmake>=2.8
  - libboost>=1.46
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
  - [fcitx-rime](https://github.com/fcitx/fcitx-rime): Fcitx frontend for Linux
  - [ibus-rime](https://github.com/rime/ibus-rime): IBus frontend for Linux
  - [Squirrel](https://github.com/rime/squirrel): frontend for Mac OS X
  - [Weasel](https://github.com/rime/weasel): frontend for Windows
  - [XIME](https://github.com/stackia/XIME): yet another Rime frontend for Mac OS X

Plugins
===
  - [librime-legacy](https://github.com/rime/librime-legacy) Legacy module with GPL-licensed code

Related works
===
  - [brise](https://github.com/rime/brise): Rime schema repository
  - Combo Pinyin: an innovative chord-typing practice to input Pinyin
  - essay: the vocabulary and language model for Rime
  - [rimekit](https://github.com/lotem/rimekit): configuration tools for Rime (under construction)
  - [SCU](https://github.com/neolee/SCU/): Squirrel Configuration Utilities

Credits
===
We are grateful to the makers of the following open source libraries:

  - [Boost C++ Libraries](http://www.boost.org/) (Boost Software License)
  - [darts-clone](https://code.google.com/p/darts-clone/) (New BSD License)
  - [google-glog](https://code.google.com/p/google-glog/) (New BSD License)
  - [Google Test](https://code.google.com/p/googletest/) (New BSD License)
  - [LevelDB](https://github.com/google/leveldb) (New BSD License)
  - [marisa-trie](https://code.google.com/p/marisa-trie/) (BSD License)
  - [OpenCC](https://github.com/BYVoid/OpenCC) (Apache License 2.0)
  - [UTF8-CPP](http://utfcpp.sourceforge.net/) (Boost Software License)
  - [yaml-cpp](https://code.google.com/p/yaml-cpp/) (MIT License)

Contributors
===
  - [佛振](https://github.com/lotem)
  - [鄒旭](https://githbu.com/zouivex)
  - [Weng Xuetian](http://csslayer.info)
  - [Chongyu Zhu](http://lembacon.com)
  - [Zhiwei Liu](https://github.com/liuzhiwei)
  - [BYVoid](http://www.byvoid.com)
  - [雪齋](https://github.com/LEOYoon-Tsaw)

