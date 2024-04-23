#ifndef RIME_DB_POOL_IMPL_H_
#define RIME_DB_POOL_IMPL_H_

#include "db_pool.h"
#include <rime/resource.h>

namespace rime {

template <class T>
DbPool<T>::DbPool(the<ResourceResolver> resource_resolver)
    : resource_resolver_(std::move(resource_resolver)) {}

template <class T>
an<T> DbPool<T>::GetDb(const string& db_name) {
  auto db = db_pool_[db_name].lock();
  if (!db) {
    auto file_path = resource_resolver_->ResolvePath(db_name);
    db = New<T>(file_path);
    db_pool_[db_name] = db;
  }
  return db;
};

}  // namespace rime

#endif  // RIME_DB_POOL_IMPL_H_
