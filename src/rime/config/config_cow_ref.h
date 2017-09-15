//
// Copyright RIME Developers
// Distributed under the BSD License
//

#ifndef RIME_CONFIG_COW_REF_H_
#define RIME_CONFIG_COW_REF_H_

#include <rime/common.h>
#include <rime/config/config_data.h>
#include <rime/config/config_types.h>

namespace rime {

template <class T>
class ConfigCowRef : public ConfigItemRef {
 public:
  ConfigCowRef(an<ConfigItemRef> parent, string key)
      : ConfigItemRef(nullptr), parent_(parent), key_(key) {
  }
  an<ConfigItem> GetItem() const override {
    auto container = As<T>(**parent_);
    return container ? Read(container, key_) : nullptr;
  }
  void SetItem(an<ConfigItem> item) override {
    auto container = As<T>(**parent_);
    if (!copied_) {
      *parent_ = container = CopyOnWrite(container, key_);
      copied_ = true;
    }
    Write(container, key_, item);
  }
 protected:
  static an<T> CopyOnWrite(const an<T>& container, const string& key);
  static an<ConfigItem> Read(const an<T>& container, const string& key);
  static void Write(const an<T>& container,
                    const string& key,
                    an<ConfigItem> value);

  an<ConfigItemRef> parent_;
  string key_;
  bool copied_ = false;
};

template <class T>
inline an<T> ConfigCowRef<T>::CopyOnWrite(const an<T>& container,
                                          const string& key) {
  if (!container) {
    DLOG(INFO) << "creating node: " << key;
    return New<T>();
  }
  DLOG(INFO) << "copy on write: " << key;
  return New<T>(*container);
}

template <>
inline an<ConfigItem> ConfigCowRef<ConfigMap>::Read(const an<ConfigMap>& map,
                                                    const string& key) {
  return map->Get(key);
}

template <>
inline void ConfigCowRef<ConfigMap>::Write(const an<ConfigMap>& map,
                                           const string& key,
                                           an<ConfigItem> value) {
  map->Set(key, value);
}

template <>
inline an<ConfigItem> ConfigCowRef<ConfigList>::Read(const an<ConfigList>& list,
                                                     const string& key) {
  return list->GetAt(ConfigData::ResolveListIndex(list, key, true));
}

template <>
inline void ConfigCowRef<ConfigList>::Write(const an<ConfigList>& list,
                                            const string& key,
                                            an<ConfigItem> value) {
  list->SetAt(ConfigData::ResolveListIndex(list, key), value);
}

inline an<ConfigItemRef> Cow(an<ConfigItemRef> parent, string key) {
  if (ConfigData::IsListItemReference(key))
    return New<ConfigCowRef<ConfigList>>(parent, key);
  else
    return New<ConfigCowRef<ConfigMap>>(parent, key);
}

}  // namespace rime

#endif  // RIME_CONFIG_COW_REF_H_
