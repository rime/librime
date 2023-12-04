#ifndef RIME_DB_POOL_H_
#define RIME_DB_POOL_H_

#include <rime/common.h>
#include <rime/resource.h>

namespace rime {

class ResourceResolver;

template <class T>
class DbPool {
 public:
  explicit DbPool(the<ResourceResolver> resource_resolver);

  an<T> GetDb(const string& db_name);

 protected:
  the<ResourceResolver> resource_resolver_;
  map<string, weak<T>> db_pool_;
};

}  // namespace rime

#endif  // RIME_DB_POOL_H_
