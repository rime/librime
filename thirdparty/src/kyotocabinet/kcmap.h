/*************************************************************************************************
 * Data mapping structures
 *                                                               Copyright (C) 2009-2012 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#ifndef _KCMAP_H                         // duplication check
#define _KCMAP_H

#include <kccommon.h>
#include <kcutil.h>

namespace kyotocabinet {                 // common namespace


/**
 * Doubly-linked hash map.
 * @param KEY the key type.
 * @param VALUE the value type.
 * @param HASH the hash functor.
 * @param EQUALTO the equality checking functor.
 */
template <class KEY, class VALUE,
          class HASH = std::hash<KEY>, class EQUALTO = std::equal_to<KEY> >
class LinkedHashMap {
 public:
  class Iterator;
 private:
  struct Record;
  /** The default bucket number of hash table. */
  static const size_t MAPDEFBNUM = 31;
  /** The mininum number of buckets to use mmap. */
  static const size_t MAPZMAPBNUM = 32768;
 public:
  /**
   * Iterator of records.
   */
  class Iterator {
    friend class LinkedHashMap;
   public:
    /**
     * Copy constructor.
     * @param src the source object.
     */
    Iterator(const Iterator& src) : map_(src.map_), rec_(src.rec_) {
      _assert_(true);
    }
    /**
     * Get the key.
     */
    const KEY& key() {
      _assert_(true);
      return rec_->key;
    }
    /**
     * Get the value.
     */
    VALUE& value() {
      _assert_(true);
      return rec_->value;
    }
    /**
     * Assignment operator from the self type.
     * @param right the right operand.
     * @return the reference to itself.
     */
    Iterator& operator =(const Iterator& right) {
      _assert_(true);
      if (&right == this) return *this;
      map_ = right.map_;
      rec_ = right.rec_;
      return *this;
    }
    /**
     * Equality operator with the self type.
     * @param right the right operand.
     * @return true if the both are equal, or false if not.
     */
    bool operator ==(const Iterator& right) const {
      _assert_(true);
      return map_ == right.map_ && rec_ == right.rec_;
    }
    /**
     * Non-equality operator with the self type.
     * @param right the right operand.
     * @return false if the both are equal, or true if not.
     */
    bool operator !=(const Iterator& right) const {
      _assert_(true);
      return map_ != right.map_ || rec_ != right.rec_;
    }
    /**
     * Preposting increment operator.
     * @return the iterator itself.
     */
    Iterator& operator ++() {
      _assert_(true);
      rec_ = rec_->next;
      return *this;
    }
    /**
     * Postpositive increment operator.
     * @return an iterator of the old position.
     */
    Iterator operator ++(int) {
      _assert_(true);
      Iterator old(*this);
      rec_ = rec_->next;
      return old;
    }
    /**
     * Preposting decrement operator.
     * @return the iterator itself.
     */
    Iterator& operator --() {
      _assert_(true);
      if (rec_) {
        rec_ = rec_->prev;
      } else {
        rec_ = map_->last_;
      }
      return *this;
    }
    /**
     * Postpositive decrement operator.
     * @return an iterator of the old position.
     */
    Iterator operator --(int) {
      _assert_(true);
      Iterator old(*this);
      if (rec_) {
        rec_ = rec_->prev;
      } else {
        rec_ = map_->last_;
      }
      return old;
    }
   private:
    /**
     * Constructor.
     * @param map the container.
     * @param rec the pointer to the current record.
     */
    explicit Iterator(LinkedHashMap* map, Record* rec) : map_(map), rec_(rec) {
      _assert_(map);
    }
    /** The container. */
    LinkedHashMap* map_;
    /** The current record. */
    Record* rec_;
  };
  /**
   * Moving Modes.
   */
  enum MoveMode {
    MCURRENT,                            ///< keep the current position
    MFIRST,                              ///< move to the first
    MLAST                                ///< move to the last
  };
  /**
   * Default constructor.
   */
  explicit LinkedHashMap() :
      buckets_(NULL), bnum_(MAPDEFBNUM), first_(NULL), last_(NULL), count_(0) {
    _assert_(true);
    initialize();
  }
  /**
   * Constructor.
   * @param bnum the number of buckets of the hash table.
   */
  explicit LinkedHashMap(size_t bnum) :
      buckets_(NULL), bnum_(bnum), first_(NULL), last_(NULL), count_(0) {
    _assert_(true);
    if (bnum_ < 1) bnum_ = MAPDEFBNUM;
    initialize();
  }
  /**
   * Destructor.
   */
  ~LinkedHashMap() {
    _assert_(true);
    destroy();
  }
  /**
   * Store a record.
   * @param key the key.
   * @param value the value.
   * @param mode the moving mode.
   * @return the pointer to the value of the stored record.
   */
  VALUE *set(const KEY& key, const VALUE& value, MoveMode mode) {
    _assert_(true);
    size_t bidx = hash_(key) % bnum_;
    Record* rec = buckets_[bidx];
    Record** entp = buckets_ + bidx;
    while (rec) {
      if (equalto_(rec->key, key)) {
        rec->value = value;
        switch (mode) {
          default: {
            break;
          }
          case MFIRST: {
            if (first_ != rec) {
              if (last_ == rec) last_ = rec->prev;
              if (rec->prev) rec->prev->next = rec->next;
              if (rec->next) rec->next->prev = rec->prev;
              rec->prev = NULL;
              rec->next = first_;
              first_->prev = rec;
              first_ = rec;
            }
            break;
          }
          case MLAST: {
            if (last_ != rec) {
              if (first_ == rec) first_ = rec->next;
              if (rec->prev) rec->prev->next = rec->next;
              if (rec->next) rec->next->prev = rec->prev;
              rec->prev = last_;
              rec->next = NULL;
              last_->next = rec;
              last_ = rec;
            }
            break;
          }
        }
        return &rec->value;
      } else {
        entp = &rec->child;
        rec = rec->child;
      }
    }
    rec = new Record(key, value);
    switch (mode) {
      default: {
        rec->prev = last_;
        if (!first_) first_ = rec;
        if (last_) last_->next = rec;
        last_ = rec;
        break;
      }
      case MFIRST: {
        rec->next = first_;
        if (!last_) last_ = rec;
        if (first_) first_->prev = rec;
        first_ = rec;
        break;
      }
    }
    *entp = rec;
    count_++;
    return &rec->value;
  }
  /**
   * Remove a record.
   * @param key the key.
   * @return true on success, or false on failure.
   */
  bool remove(const KEY& key) {
    _assert_(true);
    size_t bidx = hash_(key) % bnum_;
    Record* rec = buckets_[bidx];
    Record** entp = buckets_ + bidx;
    while (rec) {
      if (equalto_(rec->key, key)) {
        if (rec->prev) rec->prev->next = rec->next;
        if (rec->next) rec->next->prev = rec->prev;
        if (rec == first_) first_ = rec->next;
        if (rec == last_) last_ = rec->prev;
        *entp = rec->child;
        count_--;
        delete rec;
        return true;
      } else {
        entp = &rec->child;
        rec = rec->child;
      }
    }
    return false;
  }
  /**
   * Migrate a record to another map.
   * @param key the key.
   * @param dist the destination map.
   * @param mode the moving mode.
   * @return the pointer to the value of the migrated record, or NULL on failure.
   */
  VALUE* migrate(const KEY& key, LinkedHashMap* dist, MoveMode mode) {
    _assert_(dist);
    size_t hash = hash_(key);
    size_t bidx = hash % bnum_;
    Record* rec = buckets_[bidx];
    Record** entp = buckets_ + bidx;
    while (rec) {
      if (equalto_(rec->key, key)) {
        if (rec->prev) rec->prev->next = rec->next;
        if (rec->next) rec->next->prev = rec->prev;
        if (rec == first_) first_ = rec->next;
        if (rec == last_) last_ = rec->prev;
        *entp = rec->child;
        count_--;
        rec->child = NULL;
        rec->prev = NULL;
        rec->next = NULL;
        bidx = hash % dist->bnum_;
        Record* drec = dist->buckets_[bidx];
        entp = dist->buckets_ + bidx;
        while (drec) {
          if (dist->equalto_(drec->key, key)) {
            if (drec->child) rec->child = drec->child;
            if (drec->prev) {
              rec->prev = drec->prev;
              rec->prev->next = rec;
            }
            if (drec->next) {
              rec->next = drec->next;
              rec->next->prev = rec;
            }
            if (dist->first_ == drec) dist->first_ = rec;
            if (dist->last_ == drec) dist->last_ = rec;
            *entp = rec;
            delete drec;
            switch (mode) {
              default: {
                break;
              }
              case MFIRST: {
                if (dist->first_ != rec) {
                  if (dist->last_ == rec) dist->last_ = rec->prev;
                  if (rec->prev) rec->prev->next = rec->next;
                  if (rec->next) rec->next->prev = rec->prev;
                  rec->prev = NULL;
                  rec->next = dist->first_;
                  dist->first_->prev = rec;
                  dist->first_ = rec;
                }
                break;
              }
              case MLAST: {
                if (dist->last_ != rec) {
                  if (dist->first_ == rec) dist->first_ = rec->next;
                  if (rec->prev) rec->prev->next = rec->next;
                  if (rec->next) rec->next->prev = rec->prev;
                  rec->prev = dist->last_;
                  rec->next = NULL;
                  dist->last_->next = rec;
                  dist->last_ = rec;
                }
                break;
              }
            }
            return &rec->value;
          } else {
            entp = &drec->child;
            drec = drec->child;
          }
        }
        switch (mode) {
          default: {
            rec->prev = dist->last_;
            if (!dist->first_) dist->first_ = rec;
            if (dist->last_) dist->last_->next = rec;
            dist->last_ = rec;
            break;
          }
          case MFIRST: {
            rec->next = dist->first_;
            if (!dist->last_) dist->last_ = rec;
            if (dist->first_) dist->first_->prev = rec;
            dist->first_ = rec;
            break;
          }
        }
        *entp = rec;
        dist->count_++;
        return &rec->value;
      } else {
        entp = &rec->child;
        rec = rec->child;
      }
    }
    return NULL;
  }
  /**
   * Retrieve a record.
   * @param key the key.
   * @param mode the moving mode.
   * @return the pointer to the value of the corresponding record, or NULL on failure.
   */
  VALUE* get(const KEY& key, MoveMode mode) {
    _assert_(true);
    size_t bidx = hash_(key) % bnum_;
    Record* rec = buckets_[bidx];
    while (rec) {
      if (equalto_(rec->key, key)) {
        switch (mode) {
          default: {
            break;
          }
          case MFIRST: {
            if (first_ != rec) {
              if (last_ == rec) last_ = rec->prev;
              if (rec->prev) rec->prev->next = rec->next;
              if (rec->next) rec->next->prev = rec->prev;
              rec->prev = NULL;
              rec->next = first_;
              first_->prev = rec;
              first_ = rec;
            }
            break;
          }
          case MLAST: {
            if (last_ != rec) {
              if (first_ == rec) first_ = rec->next;
              if (rec->prev) rec->prev->next = rec->next;
              if (rec->next) rec->next->prev = rec->prev;
              rec->prev = last_;
              rec->next = NULL;
              last_->next = rec;
              last_ = rec;
            }
            break;
          }
        }
        return &rec->value;
      } else {
        rec = rec->child;
      }
    }
    return NULL;
  }
  /**
   * Remove all records.
   */
  void clear() {
    _assert_(true);
    if (count_ < 1) return;
    Record* rec = last_;
    while (rec) {
      Record* prev = rec->prev;
      delete rec;
      rec = prev;
    }
    for (size_t i = 0; i < bnum_; i++) {
      buckets_[i] = NULL;
    }
    first_ = NULL;
    last_ = NULL;
    count_ = 0;
  }
  /**
   * Get the number of records.
   */
  size_t count() {
    _assert_(true);
    return count_;
  }
  /**
   * Get an iterator at the first record.
   */
  Iterator begin() {
    _assert_(true);
    return Iterator(this, first_);
  }
  /**
   * Get an iterator of the end sentry.
   */
  Iterator end() {
    _assert_(true);
    return Iterator(this, NULL);
  }
  /**
   * Get an iterator at a record.
   * @param key the key.
   * @return the pointer to the value of the corresponding record, or NULL on failure.
   */
  Iterator find(const KEY& key) {
    _assert_(true);
    size_t bidx = hash_(key) % bnum_;
    Record* rec = buckets_[bidx];
    while (rec) {
      if (equalto_(rec->key, key)) {
        return Iterator(this, rec);
      } else {
        rec = rec->child;
      }
    }
    return Iterator(this, NULL);
  }
  /**
   * Get the reference of the key of the first record.
   * @return the reference of the key of the first record.
   */
  const KEY& first_key() {
    _assert_(true);
    return first_->key;
  }
  /**
   * Get the reference of the value of the first record.
   * @return the reference of the value of the first record.
   */
  VALUE& first_value() {
    _assert_(true);
    return first_->value;
  }
  /**
   * Get the reference of the key of the last record.
   * @return the reference of the key of the last record.
   */
  const KEY& last_key() {
    _assert_(true);
    return last_->key;
  }
  /**
   * Get the reference of the value of the last record.
   * @return the reference of the value of the last record.
   */
  VALUE& last_value() {
    _assert_(true);
    return last_->value;
  }
 private:
  /**
   * Record data.
   */
  struct Record {
    KEY key;                             ///< key
    VALUE value;                         ///< value
    Record* child;                       ///< child record
    Record* prev;                        ///< previous record
    Record* next;                        ///< next record
    /** constructor */
    explicit Record(const KEY& k, const VALUE& v) :
        key(k), value(v), child(NULL), prev(NULL), next(NULL) {
      _assert_(true);
    }
  };
  /**
   * Initialize fields.
   */
  void initialize() {
    _assert_(true);
    if (bnum_ >= MAPZMAPBNUM) {
      buckets_ = (Record**)mapalloc(sizeof(*buckets_) * bnum_);
    } else {
      buckets_ = new Record*[bnum_];
      for (size_t i = 0; i < bnum_; i++) {
        buckets_[i] = NULL;
      }
    }
  }
  /**
   * Clean up fields.
   */
  void destroy() {
    _assert_(true);
    Record* rec = last_;
    while (rec) {
      Record* prev = rec->prev;
      delete rec;
      rec = prev;
    }
    if (bnum_ >= MAPZMAPBNUM) {
      mapfree(buckets_);
    } else {
      delete[] buckets_;
    }
  }
  /** Dummy constructor to forbid the use. */
  LinkedHashMap(const LinkedHashMap&);
  /** Dummy Operator to forbid the use. */
  LinkedHashMap& operator =(const LinkedHashMap&);
  /** The functor of the hash function. */
  HASH hash_;
  /** The functor of the equalto function. */
  EQUALTO equalto_;
  /** The bucket array. */
  Record** buckets_;
  /** The number of buckets. */
  size_t bnum_;
  /** The first record. */
  Record* first_;
  /** The last record. */
  Record* last_;
  /** The number of records. */
  size_t count_;
};


/**
 * Memory-saving string hash map.
 */
class TinyHashMap {
 public:
  class Iterator;
 private:
  struct Record;
  struct RecordComparator;
  /** The default bucket number of hash table. */
  static const size_t MAPDEFBNUM = 31;
  /** The mininum number of buckets to use mmap. */
  static const size_t MAPZMAPBNUM = 32768;
 public:
  /**
   * Iterator of records.
   */
  class Iterator {
    friend class TinyHashMap;
   public:
    /**
     * Constructor.
     * @param map the container.
     * @note This object will not be invalidated even when the map object is updated once.
     * However, phantom records may be retrieved if they are removed after creation of each
     * iterator.
     */
    explicit Iterator(TinyHashMap* map) : map_(map), bidx_(-1), ridx_(0), recs_() {
      _assert_(map);
      step();
    }
    /**
     * Destructor.
     */
    ~Iterator() {
      _assert_(true);
      free_records();
    }
    /**
     * Get the key of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return the pointer to the key region of the current record, or NULL on failure.
     */
    const char* get_key(size_t* sp) {
      _assert_(sp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *sp = rec.ksiz_;
      return rec.kbuf_;
    }
    /**
     * Get the value of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return the pointer to the value region of the current record, or NULL on failure.
     */
    const char* get_value(size_t* sp) {
      _assert_(sp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *sp = rec.vsiz_;
      return rec.vbuf_;
    }
    /**
     * Get a pair of the key and the value of the current record.
     * @param ksp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param vbp the pointer to the variable into which the pointer to the value region is
     * assigned.
     * @param vsp the pointer to the variable into which the size of the value region is
     * assigned.
     * @return the pointer to the key region, or NULL on failure.
     */
    const char* get(size_t* ksp, const char** vbp, size_t* vsp) {
      _assert_(ksp && vbp && vsp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *ksp = rec.ksiz_;
      *vbp = rec.vbuf_;
      *vsp = rec.vsiz_;
      return rec.kbuf_;
    }
    /**
     * Step the cursor to the next record.
     */
    void step() {
      _assert_(true);
      if (++ridx_ >= recs_.size()) {
        ridx_ = 0;
        free_records();
        while (true) {
          bidx_++;
          if (bidx_ >= (int64_t)map_->bnum_) return;
          read_records();
          if (recs_.size() > 0) break;
        }
      }
    }
   private:
    /**
     * Read records of the current bucket.
     */
    void read_records() {
      char* rbuf = map_->buckets_[bidx_];
      while (rbuf) {
        Record rec(rbuf);
        size_t rsiz = sizeof(rec.child_) + sizevarnum(rec.ksiz_) + rec.ksiz_ +
            sizevarnum(rec.vsiz_) + rec.vsiz_ + sizevarnum(rec.psiz_);
        char* nbuf = new char[rsiz];
        std::memcpy(nbuf, rbuf, rsiz);
        recs_.push_back(nbuf);
        rbuf = rec.child_;
      }
    }
    /**
     * Release recources of the current records.
     */
    void free_records() {
      std::vector<char*>::iterator it = recs_.begin();
      std::vector<char*>::iterator itend = recs_.end();
      while (it != itend) {
        char* rbuf = *it;
        delete[] rbuf;
        ++it;
      }
      recs_.clear();
    }
    /** Dummy constructor to forbid the use. */
    Iterator(const Iterator&);
    /** Dummy Operator to forbid the use. */
    Iterator& operator =(const Iterator&);
    /** The container. */
    TinyHashMap* map_;
    /** The current bucket index. */
    int64_t bidx_;
    /** The current record index. */
    size_t ridx_;
    /** The current records. */
    std::vector<char*> recs_;
  };
  /**
   * Sorter of records.
   */
  class Sorter {
   public:
    /**
     * Constructor.
     * @param map the container.
     * @note This object will be invalidated when the map object is updated once.
     */
    explicit Sorter(TinyHashMap* map) : map_(map), ridx_(0), recs_() {
      _assert_(map);
      char** buckets = map_->buckets_;
      size_t bnum = map_->bnum_;
      for (size_t i = 0; i < bnum; i++) {
        char* rbuf = buckets[i];
        while (rbuf) {
          Record rec(rbuf);
          recs_.push_back(rbuf);
          rbuf = *(char**)rbuf;
        }
      }
      std::sort(recs_.begin(), recs_.end(), RecordComparator());
    }
    /**
     * Destructor.
     */
    ~Sorter() {
      _assert_(true);
    }
    /**
     * Get the key of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return the pointer to the key region of the current record, or NULL on failure.
     */
    const char* get_key(size_t* sp) {
      _assert_(sp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *sp = rec.ksiz_;
      return rec.kbuf_;
    }
    /**
     * Get the value of the current record.
     * @param sp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @return the pointer to the value region of the current record, or NULL on failure.
     */
    const char* get_value(size_t* sp) {
      _assert_(sp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *sp = rec.vsiz_;
      return rec.vbuf_;
    }
    /**
     * Get a pair of the key and the value of the current record.
     * @param ksp the pointer to the variable into which the size of the region of the return
     * value is assigned.
     * @param vbp the pointer to the variable into which the pointer to the value region is
     * assigned.
     * @param vsp the pointer to the variable into which the size of the value region is
     * assigned.
     * @return the pointer to the key region, or NULL on failure.
     */
    const char* get(size_t* ksp, const char** vbp, size_t* vsp) {
      _assert_(ksp && vbp && vsp);
      if (ridx_ >= recs_.size()) return NULL;
      Record rec(recs_[ridx_]);
      *ksp = rec.ksiz_;
      *vbp = rec.vbuf_;
      *vsp = rec.vsiz_;
      return rec.kbuf_;
    }
    /**
     * Step the cursor to the next record.
     */
    void step() {
      _assert_(true);
      ridx_++;
    }
    /** The container. */
    TinyHashMap* map_;
    /** The current record index. */
    size_t ridx_;
    /** The current records. */
    std::vector<char*> recs_;
  };
  /**
   * Default constructor.
   */
  explicit TinyHashMap() : buckets_(NULL), bnum_(MAPDEFBNUM), count_(0) {
    _assert_(true);
    initialize();
  }
  /**
   * Constructor.
   * @param bnum the number of buckets of the hash table.
   */
  explicit TinyHashMap(size_t bnum) : buckets_(NULL), bnum_(bnum), count_(0) {
    _assert_(true);
    if (bnum_ < 1) bnum_ = MAPDEFBNUM;
    initialize();
  }
  /**
   * Destructor.
   */
  ~TinyHashMap() {
    _assert_(true);
    destroy();
  }
  /**
   * Set the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the value is overwritten.
   */
  void set(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    char** entp = buckets_ + bidx;
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) {
        int32_t oh = (int32_t)sizevarnum(vsiz) - (int32_t)sizevarnum(rec.vsiz_);
        int64_t psiz = (int64_t)(rec.vsiz_ + rec.psiz_) - (int64_t)(vsiz + oh);
        if (psiz >= 0) {
          rec.overwrite(rbuf, vbuf, vsiz, psiz);
        } else {
          Record nrec(rec.child_, kbuf, ksiz, vbuf, vsiz, 0);
          delete[] rbuf;
          *entp = nrec.serialize();
        }
        return;
      }
      entp = (char**)rbuf;
      rbuf = rec.child_;
    }
    Record nrec(NULL, kbuf, ksiz, vbuf, vsiz, 0);
    *entp = nrec.serialize();
    count_++;
  }
  /**
   * Add a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the record is not modified and false is returned.
   */
  bool add(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    char** entp = buckets_ + bidx;
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) return false;
      entp = (char**)rbuf;
      rbuf = rec.child_;
    }
    Record nrec(NULL, kbuf, ksiz, vbuf, vsiz, 0);
    *entp = nrec.serialize();
    count_++;
    return true;
  }
  /**
   * Replace the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, no new record is created and false is returned.
   * If the corresponding record exists, the value is modified.
   */
  bool replace(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    char** entp = buckets_ + bidx;
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) {
        int32_t oh = (int32_t)sizevarnum(vsiz) - (int32_t)sizevarnum(rec.vsiz_);
        int64_t psiz = (int64_t)(rec.vsiz_ + rec.psiz_) - (int64_t)(vsiz + oh);
        if (psiz >= 0) {
          rec.overwrite(rbuf, vbuf, vsiz, psiz);
        } else {
          Record nrec(rec.child_, kbuf, ksiz, vbuf, vsiz, 0);
          delete[] rbuf;
          *entp = nrec.serialize();
        }
        return true;
      }
      entp = (char**)rbuf;
      rbuf = rec.child_;
    }
    return false;
  }
  /**
   * Append the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param vbuf the pointer to the value region.
   * @param vsiz the size of the value region.
   * @note If no record corresponds to the key, a new record is created.  If the corresponding
   * record exists, the given value is appended at the end of the existing value.
   */
  void append(const char* kbuf, size_t ksiz, const char* vbuf, size_t vsiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    char** entp = buckets_ + bidx;
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) {
        size_t nsiz = rec.vsiz_ + vsiz;
        int32_t oh = (int32_t)sizevarnum(nsiz) - (int32_t)sizevarnum(rec.vsiz_);
        int64_t psiz = (int64_t)(rec.vsiz_ + rec.psiz_) - (int64_t)(nsiz + oh);
        if (psiz >= 0) {
          rec.append(rbuf, oh, vbuf, vsiz, psiz);
        } else {
          psiz = nsiz + nsiz / 2;
          Record nrec(rec.child_, kbuf, ksiz, "", 0, psiz);
          char* nbuf = nrec.serialize();
          oh = (int32_t)sizevarnum(nsiz) - 1;
          psiz = (int64_t)psiz - (int64_t)(nsiz + oh);
          rec.concatenate(nbuf, rec.vbuf_, rec.vsiz_, vbuf, vsiz, psiz);
          delete[] rbuf;
          *entp = nbuf;
        }
        return;
      }
      entp = (char**)rbuf;
      rbuf = rec.child_;
    }
    Record nrec(NULL, kbuf, ksiz, vbuf, vsiz, 0);
    *entp = nrec.serialize();
    count_++;
  }
  /**
   * Remove a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return true on success, or false on failure.
   * @note If no record corresponds to the key, false is returned.
   */
  bool remove(const char* kbuf, size_t ksiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    char** entp = buckets_ + bidx;
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) {
        *entp = rec.child_;
        delete[] rbuf;
        count_--;
        return true;
      }
      entp = (char**)rbuf;
      rbuf = rec.child_;
    }
    return false;
  }
  /**
   * Retrieve the value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the value region of the corresponding record, or NULL on failure.
   */
  const char* get(const char* kbuf, size_t ksiz, size_t* sp) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && sp);
    size_t bidx = hash_record(kbuf, ksiz) % bnum_;
    char* rbuf = buckets_[bidx];
    while (rbuf) {
      Record rec(rbuf);
      if (rec.ksiz_ == ksiz && !std::memcmp(rec.kbuf_, kbuf, ksiz)) {
        *sp = rec.vsiz_;
        return rec.vbuf_;
      }
      rbuf = rec.child_;
    }
    return NULL;
  }
  /**
   * Remove all records.
   */
  void clear() {
    _assert_(true);
    if (count_ < 1) return;
    for (size_t i = 0; i < bnum_; i++) {
      char* rbuf = buckets_[i];
      while (rbuf) {
        Record rec(rbuf);
        char* child = rec.child_;
        delete[] rbuf;
        rbuf = child;
      }
      buckets_[i] = NULL;
    }
    count_ = 0;
  }
  /**
   * Get the number of records.
   * @return the number of records.
   */
  size_t count() {
    _assert_(true);
    return count_;
  }
  /**
   * Get the hash value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return the hash value.
   */
  static size_t hash_record(const char* kbuf, size_t ksiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    return hashmurmur(kbuf, ksiz);
  }
 private:
  /**
   * Record data.
   */
  struct Record {
    /** constructor */
    Record(char* child, const char* kbuf, uint64_t ksiz,
           const char* vbuf, uint64_t vsiz, uint64_t psiz) :
        child_(child), kbuf_(kbuf), ksiz_(ksiz), vbuf_(vbuf), vsiz_(vsiz), psiz_(psiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ && psiz <= MEMMAXSIZ);
    }
    /** constructor */
    Record(const char* rbuf) :
        child_(NULL), kbuf_(NULL), ksiz_(0), vbuf_(NULL), vsiz_(0), psiz_(0) {
      _assert_(rbuf);
      deserialize(rbuf);
    }
    /** overwrite the buffer */
    void overwrite(char* rbuf, const char* vbuf, size_t vsiz, size_t psiz) {
      _assert_(rbuf && vbuf && vsiz <= MEMMAXSIZ && psiz <= MEMMAXSIZ);
      char* wp = rbuf + sizeof(child_) + sizevarnum(ksiz_) + ksiz_;
      wp += writevarnum(wp, vsiz);
      std::memcpy(wp, vbuf, vsiz);
      wp += vsiz;
      writevarnum(wp, psiz);
    }
    /** append a value */
    void append(char* rbuf, int32_t oh, const char* vbuf, size_t vsiz, size_t psiz) {
      _assert_(rbuf && vbuf && vsiz <= MEMMAXSIZ && psiz <= MEMMAXSIZ);
      char* wp = rbuf + sizeof(child_) + sizevarnum(ksiz_) + ksiz_;
      if (oh > 0) {
        char* pv = wp + sizevarnum(vsiz_);
        std::memmove(pv + oh, pv, vsiz_);
        wp += writevarnum(wp, vsiz_ + vsiz);
        wp = pv + oh + vsiz_;
      } else {
        wp += writevarnum(wp, vsiz_ + vsiz);
        wp += vsiz_;
      }
      std::memcpy(wp, vbuf, vsiz);
      wp += vsiz;
      writevarnum(wp, psiz);
    }
    /** concatenate two values */
    void concatenate(char* rbuf, const char* ovbuf, size_t ovsiz,
                     const char* nvbuf, size_t nvsiz, size_t psiz) {
      _assert_(rbuf && ovbuf && ovsiz <= MEMMAXSIZ && nvbuf && nvsiz <= MEMMAXSIZ);
      char* wp = rbuf + sizeof(child_) + sizevarnum(ksiz_) + ksiz_;
      wp += writevarnum(wp, ovsiz + nvsiz);
      std::memcpy(wp, ovbuf, ovsiz);
      wp += ovsiz;
      std::memcpy(wp, nvbuf, nvsiz);
      wp += nvsiz;
      writevarnum(wp, psiz);
    }
    /** serialize data into a buffer */
    char* serialize() {
      _assert_(true);
      uint64_t rsiz = sizeof(child_) + sizevarnum(ksiz_) + ksiz_ + sizevarnum(vsiz_) + vsiz_ +
          sizevarnum(psiz_) + psiz_;
      char* rbuf = new char[rsiz];
      char* wp = rbuf;
      *(char**)wp = child_;
      wp += sizeof(child_);
      wp += writevarnum(wp, ksiz_);
      std::memcpy(wp, kbuf_, ksiz_);
      wp += ksiz_;
      wp += writevarnum(wp, vsiz_);
      std::memcpy(wp, vbuf_, vsiz_);
      wp += vsiz_;
      writevarnum(wp, psiz_);
      return rbuf;
    }
    /** deserialize a buffer into object */
    void deserialize(const char* rbuf) {
      _assert_(rbuf);
      const char* rp = rbuf;
      child_ = *(char**)rp;
      rp += sizeof(child_);
      rp += readvarnum(rp, sizeof(ksiz_), &ksiz_);
      kbuf_ = rp;
      rp += ksiz_;
      rp += readvarnum(rp, sizeof(vsiz_), &vsiz_);
      vbuf_ = rp;
      rp += vsiz_;
      readvarnum(rp, sizeof(psiz_), &psiz_);
    }
    char* child_;                        ///< region of the child
    const char* kbuf_;                   ///< region of the key
    uint64_t ksiz_;                      ///< size of the key
    const char* vbuf_;                   ///< region of the value
    uint64_t vsiz_;                      ///< size of the key
    uint64_t psiz_;                      ///< size of the padding
  };
  /**
   * Comparator for records.
   */
  struct RecordComparator {
    /** comparing operator */
    bool operator ()(char* const& abuf, char* const& bbuf) {
      const char* akbuf = abuf + sizeof(char**);
      uint64_t aksiz;
      akbuf += readvarnum(akbuf, sizeof(aksiz), &aksiz);
      const char* bkbuf = bbuf + sizeof(char**);
      uint64_t bksiz;
      bkbuf += readvarnum(bkbuf, sizeof(bksiz), &bksiz);
      uint64_t msiz = aksiz < bksiz ? aksiz : bksiz;
      for (uint64_t i = 0; i < msiz; i++) {
        if (((uint8_t*)akbuf)[i] != ((uint8_t*)bkbuf)[i])
          return ((uint8_t*)akbuf)[i] < ((uint8_t*)bkbuf)[i];
      }
      return (int32_t)aksiz < (int32_t)bksiz;
    }
  };
  /**
   * Initialize fields.
   */
  void initialize() {
    _assert_(true);
    if (bnum_ >= MAPZMAPBNUM) {
      buckets_ = (char**)mapalloc(sizeof(*buckets_) * bnum_);
    } else {
      buckets_ = new char*[bnum_];
      for (size_t i = 0; i < bnum_; i++) {
        buckets_[i] = NULL;
      }
    }
  }
  /**
   * Clean up fields.
   */
  void destroy() {
    _assert_(true);
    for (size_t i = 0; i < bnum_; i++) {
      char* rbuf = buckets_[i];
      while (rbuf) {
        Record rec(rbuf);
        char* child = rec.child_;
        delete[] rbuf;
        rbuf = child;
      }
    }
    if (bnum_ >= MAPZMAPBNUM) {
      mapfree(buckets_);
    } else {
      delete[] buckets_;
    }
  }
  /** Dummy constructor to forbid the use. */
  TinyHashMap(const TinyHashMap&);
  /** Dummy Operator to forbid the use. */
  TinyHashMap& operator =(const TinyHashMap&);
  /** The bucket array. */
  char** buckets_;
  /** The number of buckets. */
  size_t bnum_;
  /** The number of records. */
  size_t count_;
};


/**
 * Memory-saving string array list.
 */
class TinyArrayList {
 public:
  /**
   * Default constructor.
   */
  explicit TinyArrayList() : recs_() {
    _assert_(true);
  }
  /**
   * Destructor.
   */
  ~TinyArrayList() {
    _assert_(true);
    std::deque<char*>::iterator it = recs_.begin();
    std::deque<char*>::iterator itend = recs_.end();
    while (it != itend) {
      delete[] *it;
      ++it;
    }
  }
  /**
   * Insert a record at the bottom of the list.
   * @param buf the pointer to the record region.
   * @param size the size of the record region.
   */
  void push(const char* buf, size_t size) {
    _assert_(buf && size <= MEMMAXSIZ);
    size_t rsiz = sizevarnum(size) + size;
    char* rbuf = new char[rsiz];
    char* wp = rbuf + writevarnum(rbuf, size);
    std::memcpy(wp, buf, size);
    recs_.push_back(rbuf);
  }
  /**
   * Remove a record at the bottom of the list.
   * @return true if the operation success, or false if there is no record in the list.
   */
  bool pop() {
    _assert_(true);
    if (recs_.empty()) return false;
    delete[] recs_.back();
    recs_.pop_back();
    return true;
  }
  /**
   * Insert a record at the top of the list.
   * @param buf the pointer to the record region.
   * @param size the size of the record region.
   */
  void unshift(const char* buf, size_t size) {
    _assert_(buf && size <= MEMMAXSIZ);
    size_t rsiz = sizevarnum(size) + size;
    char* rbuf = new char[rsiz];
    char* wp = rbuf + writevarnum(rbuf, size);
    std::memcpy(wp, buf, size);
    recs_.push_front(rbuf);
  }
  /**
   * Remove a record at the top of the list.
   * @return true if the operation success, or false if there is no record in the list.
   */
  bool shift() {
    _assert_(true);
    if (recs_.empty()) return false;
    delete[] recs_.front();
    recs_.pop_front();
    return true;
  }
  /**
   * Insert a record at the position of the given index of the list.
   * @param buf the pointer to the record region.
   * @param size the size of the record region.
   * @param idx the index of the position.  It must be equal to or less than the number of
   * records.
   */
  void insert(const char* buf, size_t size, size_t idx) {
    size_t rsiz = sizevarnum(size) + size;
    char* rbuf = new char[rsiz];
    char* wp = rbuf + writevarnum(rbuf, size);
    std::memcpy(wp, buf, size);
    recs_.insert(recs_.begin() + idx, rbuf);
  }
  /**
   * Remove a record at the position of the given index of the list.
   * @param idx the index of the position.  It must be less than the number of records.
   */
  void remove(size_t idx) {
    _assert_(true);
    std::deque<char*>::iterator it = recs_.begin() + idx;
    delete[] *it;
    recs_.erase(it);
  }
  /**
   * Retrieve a record at the position of the given index of the list.
   * @param idx the index of the position.  It must be less than the number of records.
   * @param sp the pointer to the variable into which the size of the region of the return
   * value is assigned.
   * @return the pointer to the region of the retrieved record.
   */
  const char* get(size_t idx, size_t* sp) {
    _assert_(sp);
    const char* rbuf = recs_[idx];
    uint64_t rsiz;
    const char* rp = rbuf + readvarnum(rbuf, sizeof(uint64_t), &rsiz);
    *sp = rsiz;
    return rp;
  }
  /**
   * Remove all records.
   */
  void clear() {
    _assert_(true);
    std::deque<char*>::iterator it = recs_.begin();
    std::deque<char*>::iterator itend = recs_.end();
    while (it != itend) {
      delete[] *it;
      ++it;
    }
    recs_.clear();
  }
  /**
   * Get the number of records.
   * @return the number of records.
   */
  size_t count() {
    _assert_(true);
    return recs_.size();
  }
 private:
  /** Dummy constructor to forbid the use. */
  TinyArrayList(const TinyArrayList&);
  /** Dummy Operator to forbid the use. */
  TinyArrayList& operator =(const TinyArrayList&);
  /** The record list. */
  std::deque<char*> recs_;
};


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
