<meta charset="UTF-8">

librime-legacy
===
Legacy module for Rime, with GPL-licensed components.

This is not a standalone library but works with [librime](https://github.com/lotem/librime) as a plugin.

Project home
---
https://github.com/lotem/librime-legacy

License
---
GPLv3

Features
===
  - `treedb`, an implementation of Rime’s user DB based on Kyoto Cabinet
  - `legacy_userdb`, an alias of `treedb`, used by `user_dict_upgrade` task to convert legacy user dictionary files to newer format

Install
===

Build dependencies
---
  - compiler with C++11 support
  - cmake>=2.8
  - libboost>=1.46
  - libglog
  - libkyotocabinet
  - librime>=1.3

Runtime dependencies
---
  - libboost
  - libglog
  - libkyotocabinet
  - librime>=1.3

Build and install
---
```
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make
sudo make install
```

Usage
===

### For user dictionary upgrade
Just load the shared library `librime-legacy.so` into your input method application,
and a module named `legacy` will be automatically registered and made available to librime.

When the deployment task `user_dict_upgrade` is run, Rime will try to load `legacy` module and
instantiate `legacy_userdb`s when converting `*.userdb.kct` format user dictionaries to newer standard.

### Use treedb component
If you wish to use `treedb` in your own Rime module, you should load `legacy` module before using the components.
``` C++
#include <rime_api.h>
#include <rime/setup.h>

static RIME_MODULE_LIST(required_modules, "legacy");
rime::LoadModules(required_modules);

rime::UserDb::Component* component = rime::UserDb::Require("treedb");
if (component) {
  rime::Db* db = component->Create("my_db");
  // ...
  delete db;
}
```


Credits
===
We are grateful to the makers of the following open source libraries:

  - [Boost C++ Libraries](http://www.boost.org/) (Boost Software License)
  - [google-glog](https://code.google.com/p/google-glog/) (New BSD License)
  - [Kyoto Cabinet](http://fallabs.com/kyotocabinet/) (GNU General Public License)

Contributors
===
  - [佛振](https://github.com/lotem)

