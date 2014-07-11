/*************************************************************************************************
 * Plant database
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


#ifndef _KCPLANTDB_H                     // duplication check
#define _KCPLANTDB_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>
#include <kcfile.h>
#include <kccompress.h>
#include <kccompare.h>
#include <kcmap.h>
#include <kcregex.h>
#include <kcdb.h>

#define KCPDBMETAKEY  "@"                ///< key of the record for meta data
#define KCPDBTMPPATHEXT  "tmpkct"        ///< extension of the temporary file
#define KCPDRECBUFSIZ  128               ///< size of the record buffer

namespace kyotocabinet {                 // common namespace


/**
 * Plant database.
 * @param BASEDB a class compatible with the file hash database class.
 * @param DBTYPE the database type number of the class.
 * @note This class template is a template for concrete classes to operate tree databases.
 * Template instance classes can be inherited but overwriting methods is forbidden.  The class
 * TreeDB is the instance of the file tree database.  The class ForestDB is the instance of the
 * directory tree database.  Before every database operation, it is necessary to call the
 * BasicDB::open method in order to open a database file and connect the database object to it.
 * To avoid data missing or corruption, it is important to close every database file by the
 * BasicDB::close method when the database is no longer in use.  It is forbidden for multible
 * database objects in a process to open the same database at the same time.  It is forbidden to
 * share a database object with child processes.
 */
template <class BASEDB, uint8_t DBTYPE>
class PlantDB : public BasicDB {
 public:
  class Cursor;
 private:
  struct Record;
  struct RecordComparator;
  struct LeafNode;
  struct Link;
  struct LinkComparator;
  struct InnerNode;
  struct LeafSlot;
  struct InnerSlot;
  class ScopedVisitor;
  /** An alias of array of records. */
  typedef std::vector<Record*> RecordArray;
  /** An alias of array of records. */
  typedef std::vector<Link*> LinkArray;
  /** An alias of leaf node cache. */
  typedef LinkedHashMap<int64_t, LeafNode*> LeafCache;
  /** An alias of inner node cache. */
  typedef LinkedHashMap<int64_t, InnerNode*> InnerCache;
  /** An alias of list of cursors. */
  typedef std::list<Cursor*> CursorList;
  /** The number of cache slots. */
  static const int32_t SLOTNUM = 16;
  /** The default alignment power. */
  static const uint8_t DEFAPOW = 8;
  /** The default free block pool power. */
  static const uint8_t DEFFPOW = 10;
  /** The default bucket number. */
  static const int64_t DEFBNUM = 64LL << 10;
  /** The default page size. */
  static const int32_t DEFPSIZ = 8192;
  /** The default capacity size of the page cache. */
  static const int64_t DEFPCCAP = 64LL << 20;
  /** The size of the header. */
  static const int64_t HEADSIZ = 80;
  /** The offset of the numbers. */
  static const int64_t MOFFNUMS = 8;
  /** The prefix of leaf nodes. */
  static const char LNPREFIX = 'L';
  /** The prefix of inner nodes. */
  static const char INPREFIX = 'I';
  /** The average number of ways of each node. */
  static const size_t AVGWAY = 16;
  /** The ratio of the warm cache. */
  static const size_t WARMRATIO = 4;
  /** The ratio of flushing inner nodes. */
  static const size_t INFLRATIO = 32;
  /** The default number of items in each leaf node. */
  static const size_t DEFLINUM = 64;
  /** The default number of items in each inner node. */
  static const size_t DEFIINUM = 128;
  /** The base ID number for inner nodes. */
  static const int64_t INIDBASE = 1LL << 48;
  /** The minimum number of links in each inner node. */
  static const size_t INLINKMIN = 8;
  /** The maximum level of B+ tree. */
  static const int32_t LEVELMAX = 16;
  /** The number of cached nodes for auto transaction. */
  static const int32_t ATRANCNUM = 256;
  /** The threshold of busy loop and sleep for locking. */
  static const uint32_t LOCKBUSYLOOP = 8192;
 public:
  /**
   * Cursor to indicate a record.
   */
  class Cursor : public BasicDB::Cursor {
    friend class PlantDB;
   public:
    /**
     * Constructor.
     * @param db the container database object.
     */
    explicit Cursor(PlantDB* db) :
        db_(db), stack_(), kbuf_(NULL), ksiz_(0), lid_(0), back_(false) {
      _assert_(db);
      ScopedRWLock lock(&db_->mlock_, true);
      db_->curs_.push_back(this);
    }
    /**
     * Destructor.
     */
    virtual ~Cursor() {
      _assert_(true);
      if (!db_) return;
      ScopedRWLock lock(&db_->mlock_, true);
      if (kbuf_) clear_position();
      db_->curs_.remove(this);
    }
    /**
     * Accept a visitor to the current record.
     * @param visitor a visitor object.
     * @param writable true for writable operation, or false for read-only operation.
     * @param step true to move the cursor to the next record, or false for no move.
     * @return true on success, or false on failure.
     * @note The operation for each record is performed atomically and other threads accessing
     * the same record are blocked.  To avoid deadlock, any explicit database operation must not
     * be performed in this function.
     */
    bool accept(Visitor* visitor, bool writable = true, bool step = false) {
      _assert_(visitor);
      bool wrlock = writable && (db_->tran_ || db_->autotran_);
      if (wrlock) {
        db_->mlock_.lock_writer();
      } else {
        db_->mlock_.lock_reader();
      }
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        db_->mlock_.unlock();
        return false;
      }
      if (writable && !(db_->writer_)) {
        db_->set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        db_->mlock_.unlock();
        return false;
      }
      if (!kbuf_) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        db_->mlock_.unlock();
        return false;
      }
      bool err = false;
      bool hit = false;


      if (lid_ > 0 && !accept_spec(visitor, writable, step, &hit)) err = true;


      if (!err && !hit) {
        if (!wrlock) {
          db_->mlock_.unlock();
          db_->mlock_.lock_writer();
        }
        if (kbuf_) {
          bool retry = true;
          while (!err && retry) {
            if (!accept_atom(visitor, step, &retry)) err = true;
          }
        } else {
          db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
          err = true;
        }
      }
      db_->mlock_.unlock();
      return !err;
    }
    /**
     * Jump the cursor to the first record for forward scan.
     * @return true on success, or false on failure.
     */
    bool jump() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, false);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      back_ = false;
      if (kbuf_) clear_position();
      bool err = false;
      if (!set_position(db_->first_)) err = true;
      return !err;
    }
    /**
     * Jump the cursor to a record for forward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    bool jump(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedRWLock lock(&db_->mlock_, false);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      back_ = false;
      if (kbuf_) clear_position();
      set_position(kbuf, ksiz, 0);
      bool err = false;
      if (!adjust_position()) {
        if (kbuf_) clear_position();
        err = true;
      }
      return !err;
    }
    /**
     * Jump the cursor to a record for forward scan.
     * @note Equal to the original Cursor::jump method except that the parameter is std::string.
     */
    bool jump(const std::string& key) {
      _assert_(true);
      return jump(key.c_str(), key.size());
    }
    /**
     * Jump the cursor to the last record for backward scan.
     * @return true on success, or false on failure.
     * @note This method is dedicated to tree databases.  Some database types, especially hash
     * databases, may provide a dummy implementation.
     */
    bool jump_back() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, false);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      back_ = true;
      if (kbuf_) clear_position();
      bool err = false;
      if (!set_position_back(db_->last_)) err = true;
      return !err;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    bool jump_back(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedRWLock lock(&db_->mlock_, false);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      back_ = true;
      if (kbuf_) clear_position();
      set_position(kbuf, ksiz, 0);
      bool err = false;
      if (adjust_position()) {
        if (db_->reccomp_.comp->compare(kbuf, ksiz, kbuf_, ksiz_) < 0) {
          bool hit = false;
          if (lid_ > 0 && !back_position_spec(&hit)) err = true;
          if (!err && !hit) {
            db_->mlock_.unlock();
            db_->mlock_.lock_writer();
            if (kbuf_) {
              if (!back_position_atom()) err = true;
            } else {
              db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
              err = true;
            }
          }
        }
      } else {
        if (kbuf_) clear_position();
        if (!set_position_back(db_->last_)) err = true;
      }
      return !err;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @note Equal to the original Cursor::jump_back method except that the parameter is
     * std::string.
     */
    bool jump_back(const std::string& key) {
      _assert_(true);
      return jump_back(key.c_str(), key.size());
    }
    /**
     * Step the cursor to the next record.
     * @return true on success, or false on failure.
     */
    bool step() {
      _assert_(true);
      back_ = false;
      DB::Visitor visitor;
      if (!accept(&visitor, false, true)) return false;
      if (!kbuf_) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      return true;
    }
    /**
     * Step the cursor to the previous record.
     * @return true on success, or false on failure.
     */
    bool step_back() {
      _assert_(true);
      db_->mlock_.lock_reader();
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        db_->mlock_.unlock();
        return false;
      }
      if (!kbuf_) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        db_->mlock_.unlock();
        return false;
      }
      back_ = true;
      bool err = false;
      bool hit = false;
      if (lid_ > 0 && !back_position_spec(&hit)) err = true;
      if (!err && !hit) {
        db_->mlock_.unlock();
        db_->mlock_.lock_writer();
        if (kbuf_) {
          if (!back_position_atom()) err = true;
        } else {
          db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
          err = true;
        }
      }
      db_->mlock_.unlock();
      return !err;
    }
    /**
     * Get the database object.
     * @return the database object.
     */
    PlantDB* db() {
      _assert_(true);
      return db_;
    }
   private:
    /**
     * Clear the position.
     */
    void clear_position() {
      _assert_(true);
      if (kbuf_ != stack_) delete[] kbuf_;
      kbuf_ = NULL;
      lid_ = 0;
    }
    /**
     * Set the current position.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @param id the ID of the current node.
     */
    void set_position(const char* kbuf, size_t ksiz, int64_t id) {
      _assert_(kbuf);
      kbuf_ = ksiz > sizeof(stack_) ? new char[ksiz] : stack_;
      ksiz_ = ksiz;
      std::memcpy(kbuf_, kbuf, ksiz);
      lid_ = id;
    }
    /**
     * Set the current position with a record.
     * @param rec the current record.
     * @param id the ID of the current node.
     */
    void set_position(Record* rec, int64_t id) {
      _assert_(rec);
      char* dbuf = (char*)rec + sizeof(*rec);
      set_position(dbuf, rec->ksiz, id);
    }
    /**
     * Set the current position at the next node.
     * @param id the ID of the next node.
     * @return true on success, or false on failure.
     */
    bool set_position(int64_t id) {
      _assert_(true);
      while (id > 0) {
        LeafNode* node = db_->load_leaf_node(id, false);
        if (!node) {
          db_->set_error(_KCCODELINE_, Error::BROKEN, "missing leaf node");
          db_->db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)id);
          return false;
        }
        ScopedRWLock lock(&node->lock, false);
        RecordArray& recs = node->recs;
        if (!recs.empty()) {
          set_position(recs.front(), id);
          return true;
        } else {
          id = node->next;
        }
      }
      db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    /**
     * Set the current position at the previous node.
     * @param id the ID of the previous node.
     * @return true on success, or false on failure.
     */
    bool set_position_back(int64_t id) {
      _assert_(true);
      while (id > 0) {
        LeafNode* node = db_->load_leaf_node(id, false);
        if (!node) {
          db_->set_error(_KCCODELINE_, Error::BROKEN, "missing leaf node");
          db_->db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)id);
          return false;
        }
        ScopedRWLock lock(&node->lock, false);
        RecordArray& recs = node->recs;
        if (!recs.empty()) {
          set_position(recs.back(), id);
          return true;
        } else {
          id = node->prev;
        }
      }
      db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
    }
    /**
     * Accept a visitor to the current record speculatively.
     * @param visitor a visitor object.
     * @param writable true for writable operation, or false for read-only operation.
     * @param step true to move the cursor to the next record, or false for no move.
     * @param hitp the pointer to the variable for the hit flag.
     * @return true on success, or false on failure.
     */
    bool accept_spec(Visitor* visitor, bool writable, bool step, bool* hitp) {
      _assert_(visitor && hitp);
      bool err = false;
      bool hit = false;
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz_;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz_;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
      LeafNode* node = db_->load_leaf_node(lid_, false);
      if (node) {
        char lstack[KCPDRECBUFSIZ];
        char* lbuf = NULL;
        size_t lsiz = 0;
        Link* link = NULL;
        int64_t hist[LEVELMAX];
        int32_t hnum = 0;
        if (writable) {
          node->lock.lock_writer();
        } else {
          node->lock.lock_reader();
        }
        RecordArray& recs = node->recs;
        if (!recs.empty()) {
          Record* frec = recs.front();
          Record* lrec = recs.back();
          if (!db_->reccomp_(rec, frec) && !db_->reccomp_(lrec, rec)) {
            typename RecordArray::iterator ritend = recs.end();
            typename RecordArray::iterator rit = std::lower_bound(recs.begin(), ritend,
                                                                  rec, db_->reccomp_);
            if (rit != ritend) {
              hit = true;
              if (db_->reccomp_(rec, *rit)) {
                clear_position();
                set_position(*rit, node->id);
                if (rbuf != rstack) delete[] rbuf;
                rsiz = sizeof(Record) + ksiz_;
                rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
                rec = (Record*)rbuf;
                rec->ksiz = ksiz_;
                rec->vsiz = 0;
                std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
              }
              rec = *rit;
              char* kbuf = (char*)rec + sizeof(*rec);
              size_t ksiz = rec->ksiz;
              size_t vsiz;
              const char* vbuf = visitor->visit_full(kbuf, ksiz, kbuf + ksiz,
                                                     rec->vsiz, &vsiz);
              if (vbuf == Visitor::REMOVE) {
                rsiz = sizeof(*rec) + rec->ksiz + rec->vsiz;
                db_->count_ -= 1;
                db_->cusage_ -= rsiz;
                node->size -= rsiz;
                node->dirty = true;
                if (recs.size() <= 1) {
                  lsiz = sizeof(Link) + ksiz;
                  lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
                  link = (Link*)lbuf;
                  link->child = 0;
                  link->ksiz = ksiz;
                  std::memcpy(lbuf + sizeof(*link), kbuf, ksiz);
                }
                xfree(rec);
                if (back_) {
                  if (rit == recs.begin()) {
                    step = true;
                  } else {
                    typename RecordArray::iterator ritprev = rit - 1;
                    set_position(*ritprev, node->id);
                    step = false;
                  }
                } else {
                  typename RecordArray::iterator ritnext = rit + 1;
                  if (ritnext == ritend) {
                    step = true;
                  } else {
                    clear_position();
                    set_position(*ritnext, node->id);
                    step = false;
                  }
                }
                recs.erase(rit);
              } else if (vbuf != Visitor::NOP) {
                int64_t diff = (int64_t)vsiz - (int64_t)rec->vsiz;
                db_->cusage_ += diff;
                node->size += diff;
                node->dirty = true;
                if (vsiz > rec->vsiz) {
                  *rit = (Record*)xrealloc(rec, sizeof(*rec) + rec->ksiz + vsiz);
                  rec = *rit;
                  kbuf = (char*)rec + sizeof(*rec);
                }
                std::memcpy(kbuf + rec->ksiz, vbuf, vsiz);
                rec->vsiz = vsiz;
                if (node->size > db_->psiz_ && recs.size() > 1) {
                  lsiz = sizeof(Link) + ksiz;
                  lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
                  link = (Link*)lbuf;
                  link->child = 0;
                  link->ksiz = ksiz;
                  std::memcpy(lbuf + sizeof(*link), kbuf, ksiz);
                }
              }
              if (step) {
                if (back_) {
                  if (rit != recs.begin()) {
                    --rit;
                    set_position(*rit, node->id);
                    step = false;
                  }
                } else {
                  ++rit;
                  if (rit != ritend) {
                    clear_position();
                    set_position(*rit, node->id);
                    step = false;
                  }
                }
              }
            }
          }
        }
        bool atran = db_->autotran_ && !db_->tran_ && node->dirty;
        bool async = db_->autosync_ && !db_->autotran_ && !db_->tran_ && node->dirty;
        node->lock.unlock();
        if (hit && step) {
          clear_position();
          if (back_) {
            set_position_back(node->prev);
          } else {
            set_position(node->next);
          }
        }
        if (hit) {
          bool flush = db_->cusage_ > db_->pccap_;
          if (link || flush || async) {
            int64_t id = node->id;
            if (atran && !link && !db_->fix_auto_transaction_leaf(node)) err = true;
            db_->mlock_.unlock();
            db_->mlock_.lock_writer();
            if (link) {
              node = db_->search_tree(link, true, hist, &hnum);
              if (node) {
                if (!db_->reorganize_tree(node, hist, hnum)) err = true;
                if (atran && !db_->tran_ && !db_->fix_auto_transaction_tree()) err = true;
              } else {
                db_->set_error(_KCCODELINE_, Error::BROKEN, "search failed");
                err = true;
              }
            } else if (flush) {
              int32_t idx = id % SLOTNUM;
              LeafSlot* lslot = db_->lslots_ + idx;
              if (!db_->flush_leaf_cache_part(lslot)) err = true;
              InnerSlot* islot = db_->islots_ + idx;
              if (islot->warm->count() > lslot->warm->count() + lslot->hot->count() + 1 &&
                  !db_->flush_inner_cache_part(islot)) err = true;
            }
            if (async && !db_->fix_auto_synchronization()) err = true;
          } else {
            if (!db_->fix_auto_transaction_leaf(node)) err = true;
          }
        }
        if (lbuf != lstack) delete[] lbuf;
      }
      if (rbuf != rstack) delete[] rbuf;
      *hitp = hit;
      return !err;
    }
    /**
     * Accept a visitor to the current record atomically.
     * @param visitor a visitor object.
     * @param step true to move the cursor to the next record, or false for no move.
     * @param retryp the pointer to the variable for the retry flag.
     * @return true on success, or false on failure.
     */
    bool accept_atom(Visitor* visitor, bool step, bool *retryp) {
      _assert_(visitor && retryp);
      bool err = false;
      bool reorg = false;
      *retryp = false;
      char lstack[KCPDRECBUFSIZ];
      size_t lsiz = sizeof(Link) + ksiz_;
      char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
      Link* link = (Link*)lbuf;
      link->child = 0;
      link->ksiz = ksiz_;
      std::memcpy(lbuf + sizeof(*link), kbuf_, ksiz_);
      int64_t hist[LEVELMAX];
      int32_t hnum = 0;
      LeafNode* node = db_->search_tree(link, true, hist, &hnum);
      if (!node) {
        db_->set_error(_KCCODELINE_, Error::BROKEN, "search failed");
        if (lbuf != lstack) delete[] lbuf;
        return false;
      }
      if (node->recs.empty()) {
        if (lbuf != lstack) delete[] lbuf;
        clear_position();
        if (!set_position(node->next)) return false;
        node = db_->load_leaf_node(lid_, false);
        if (!node) {
          db_->set_error(_KCCODELINE_, Error::BROKEN, "search failed");
          return false;
        }
        lsiz = sizeof(Link) + ksiz_;
        char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
        Link* link = (Link*)lbuf;
        link->child = 0;
        link->ksiz = ksiz_;
        std::memcpy(lbuf + sizeof(*link), kbuf_, ksiz_);
        node = db_->search_tree(link, true, hist, &hnum);
        if (node->id != lid_) {
          db_->set_error(_KCCODELINE_, Error::BROKEN, "invalid tree");
          if (lbuf != lstack) delete[] lbuf;
          return false;
        }
      }
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz_;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz_;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
      RecordArray& recs = node->recs;
      typename RecordArray::iterator ritend = recs.end();
      typename RecordArray::iterator rit = std::lower_bound(recs.begin(), ritend,
                                                            rec, db_->reccomp_);
      if (rit != ritend) {
        if (db_->reccomp_(rec, *rit)) {
          clear_position();
          set_position(*rit, node->id);
          if (rbuf != rstack) delete[] rbuf;
          rsiz = sizeof(Record) + ksiz_;
          rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
          rec = (Record*)rbuf;
          rec->ksiz = ksiz_;
          rec->vsiz = 0;
          std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
        }
        rec = *rit;
        char* kbuf = (char*)rec + sizeof(*rec);
        size_t ksiz = rec->ksiz;
        size_t vsiz;
        const char* vbuf = visitor->visit_full(kbuf, ksiz, kbuf + ksiz,
                                               rec->vsiz, &vsiz);
        if (vbuf == Visitor::REMOVE) {
          rsiz = sizeof(*rec) + rec->ksiz + rec->vsiz;
          db_->count_ -= 1;
          db_->cusage_ -= rsiz;
          node->size -= rsiz;
          node->dirty = true;
          xfree(rec);
          step = false;
          clear_position();
          if (back_) {
            if (rit == recs.begin()) {
              set_position_back(node->prev);
            } else {
              typename RecordArray::iterator ritprev = rit - 1;
              set_position(*ritprev, node->id);
            }
          } else {
            typename RecordArray::iterator ritnext = rit + 1;
            if (ritnext == ritend) {
              set_position(node->next);
            } else {
              set_position(*ritnext, node->id);
            }
          }
          recs.erase(rit);
          if (recs.empty()) reorg = true;
        } else if (vbuf != Visitor::NOP) {
          int64_t diff = (int64_t)vsiz - (int64_t)rec->vsiz;
          db_->cusage_ += diff;
          node->size += diff;
          node->dirty = true;
          if (vsiz > rec->vsiz) {
            *rit = (Record*)xrealloc(rec, sizeof(*rec) + rec->ksiz + vsiz);
            rec = *rit;
            kbuf = (char*)rec + sizeof(*rec);
          }
          std::memcpy(kbuf + rec->ksiz, vbuf, vsiz);
          rec->vsiz = vsiz;
          if (node->size > db_->psiz_ && recs.size() > 1) reorg = true;
        }
        if (step) {
          clear_position();
          if (back_) {
            if (rit == recs.begin()) {
              set_position_back(node->prev);
            } else {
              --rit;
              set_position(*rit, node->id);
            }
          } else {
            ++rit;
            if (rit == ritend) {
              set_position(node->next);
            } else {
              set_position(*rit, node->id);
            }
          }
        }
        bool atran = db_->autotran_ && !db_->tran_ && node->dirty;
        bool async = db_->autosync_ && !db_->autotran_ && !db_->tran_ && node->dirty;
        if (atran && !reorg && !db_->fix_auto_transaction_leaf(node)) err = true;
        if (reorg) {
          if (!db_->reorganize_tree(node, hist, hnum)) err = true;
          if (atran && !db_->fix_auto_transaction_tree()) err = true;
        } else if (db_->cusage_ > db_->pccap_) {
          int32_t idx = node->id % SLOTNUM;
          LeafSlot* lslot = db_->lslots_ + idx;
          if (!db_->flush_leaf_cache_part(lslot)) err = true;
          InnerSlot* islot = db_->islots_ + idx;
          if (islot->warm->count() > lslot->warm->count() + lslot->hot->count() + 1 &&
              !db_->flush_inner_cache_part(islot)) err = true;
        }
        if (async && !db_->fix_auto_synchronization()) err = true;
      } else {
        int64_t lid = lid_;
        clear_position();
        if (back_) {
          if (set_position_back(node->prev)) {
            if (lid_ == lid) {
              db_->set_error(_KCCODELINE_, Error::BROKEN, "invalid leaf node");
              err = true;
            } else {
              *retryp = true;
            }
          } else {
            db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
            err = true;
          }
        } else {
          if (set_position(node->next)) {
            if (lid_ == lid) {
              db_->set_error(_KCCODELINE_, Error::BROKEN, "invalid leaf node");
              err = true;
            } else {
              *retryp = true;
            }
          } else {
            db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
            err = true;
          }
        }
      }
      if (rbuf != rstack) delete[] rbuf;
      if (lbuf != lstack) delete[] lbuf;
      return !err;
    }
    /**
     * Adjust the position to an existing record.
     * @return true on success, or false on failure.
     */
    bool adjust_position() {
      _assert_(true);
      char lstack[KCPDRECBUFSIZ];
      size_t lsiz = sizeof(Link) + ksiz_;
      char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
      Link* link = (Link*)lbuf;
      link->child = 0;
      link->ksiz = ksiz_;
      std::memcpy(lbuf + sizeof(*link), kbuf_, ksiz_);
      int64_t hist[LEVELMAX];
      int32_t hnum = 0;
      LeafNode* node = db_->search_tree(link, true, hist, &hnum);
      if (!node) {
        db_->set_error(_KCCODELINE_, Error::BROKEN, "search failed");
        if (lbuf != lstack) delete[] lbuf;
        return false;
      }
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz_;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz_;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
      bool err = false;
      node->lock.lock_reader();
      const RecordArray& recs = node->recs;
      typename RecordArray::const_iterator ritend = node->recs.end();
      typename RecordArray::const_iterator rit = std::lower_bound(recs.begin(), ritend,
                                                                  rec, db_->reccomp_);
      clear_position();
      if (rit == ritend) {
        node->lock.unlock();
        if (!set_position(node->next)) err = true;
      } else {
        set_position(*rit, node->id);
        node->lock.unlock();
      }
      if (rbuf != rstack) delete[] rbuf;
      if (lbuf != lstack) delete[] lbuf;
      return !err;
    }
    /**
     * Back the position to the previous record speculatively.
     * @param hitp the pointer to the variable for the hit flag.
     * @return true on success, or false on failure.
     */
    bool back_position_spec(bool* hitp) {
      _assert_(hitp);
      bool err = false;
      bool hit = false;
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz_;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz_;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
      LeafNode* node = db_->load_leaf_node(lid_, false);
      if (node) {
        node->lock.lock_reader();
        RecordArray& recs = node->recs;
        if (recs.empty()) {
          node->lock.unlock();
        } else {
          Record* frec = recs.front();
          Record* lrec = recs.back();
          if (db_->reccomp_(rec, frec)) {
            hit = true;
            clear_position();
            node->lock.unlock();
            if (!set_position_back(node->prev)) err = true;
          } else if (db_->reccomp_(lrec, rec)) {
            node->lock.unlock();
          } else {
            hit = true;
            typename RecordArray::iterator ritbeg = recs.begin();
            typename RecordArray::iterator ritend = recs.end();
            typename RecordArray::iterator rit = std::lower_bound(recs.begin(), ritend,
                                                                  rec, db_->reccomp_);
            clear_position();
            if (rit == ritbeg) {
              node->lock.unlock();
              if (!set_position_back(node->prev)) err = true;
            } else {
              --rit;
              set_position(*rit, node->id);
              node->lock.unlock();
            }
          }
        }
      }
      if (rbuf != rstack) delete[] rbuf;
      *hitp = hit;
      return !err;
    }
    /**
     * Back the position to the previous record atomically.
     * @return true on success, or false on failure.
     */
    bool back_position_atom() {
      _assert_(true);
      char lstack[KCPDRECBUFSIZ];
      size_t lsiz = sizeof(Link) + ksiz_;
      char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
      Link* link = (Link*)lbuf;
      link->child = 0;
      link->ksiz = ksiz_;
      std::memcpy(lbuf + sizeof(*link), kbuf_, ksiz_);
      int64_t hist[LEVELMAX];
      int32_t hnum = 0;
      LeafNode* node = db_->search_tree(link, true, hist, &hnum);
      if (!node) {
        db_->set_error(_KCCODELINE_, Error::BROKEN, "search failed");
        if (lbuf != lstack) delete[] lbuf;
        return false;
      }
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz_;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz_;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf_, ksiz_);
      bool err = false;
      node->lock.lock_reader();
      const RecordArray& recs = node->recs;
      typename RecordArray::const_iterator ritbeg = node->recs.begin();
      typename RecordArray::const_iterator ritend = node->recs.end();
      typename RecordArray::const_iterator rit = std::lower_bound(recs.begin(), ritend,
                                                                  rec, db_->reccomp_);
      clear_position();
      if (rit == ritbeg) {
        node->lock.unlock();
        if (!set_position_back(node->prev)) err = true;
      } else if (rit == ritend) {
        ritend--;
        set_position(*ritend, node->id);
        node->lock.unlock();
      } else {
        --rit;
        set_position(*rit, node->id);
        node->lock.unlock();
      }
      if (rbuf != rstack) delete[] rbuf;
      if (lbuf != lstack) delete[] lbuf;
      return !err;
    }
    /** Dummy constructor to forbid the use. */
    Cursor(const Cursor&);
    /** Dummy Operator to forbid the use. */
    Cursor& operator =(const Cursor&);
    /** The inner database. */
    PlantDB* db_;
    /** The stack buffer for the key. */
    char stack_[KCPDRECBUFSIZ];
    /** The pointer to the key region. */
    char* kbuf_;
    /** The size of the key region. */
    size_t ksiz_;
    /** The last visited leaf. */
    int64_t lid_;
    /** The backward flag. */
    bool back_;
  };
  /**
   * Tuning options.
   */
  enum Option {
    TSMALL = BASEDB::TSMALL,             ///< use 32-bit addressing
    TLINEAR = BASEDB::TLINEAR,           ///< use linear collision chaining
    TCOMPRESS = BASEDB::TCOMPRESS        ///< compress each record
  };
  /**
   * Status flags.
   */
  enum Flag {
    FOPEN = BASEDB::FOPEN,               ///< whether opened
    FFATAL = BASEDB::FFATAL              ///< whether with fatal error
  };
  /**
   * Default constructor.
   */
  explicit PlantDB() :
      mlock_(), mtrigger_(NULL), omode_(0), writer_(false), autotran_(false), autosync_(false),
      db_(), curs_(), apow_(DEFAPOW), fpow_(DEFFPOW), opts_(0), bnum_(DEFBNUM),
      psiz_(DEFPSIZ), pccap_(DEFPCCAP),
      root_(0), first_(0), last_(0), lcnt_(0), icnt_(0), count_(0), cusage_(0),
      lslots_(), islots_(), reccomp_(), linkcomp_(),
      tran_(false), trclock_(0), trlcnt_(0), trcount_(0) {
    _assert_(true);
  }
  /**
   * Destructor.
   * @note If the database is not closed, it is closed implicitly.
   */
  virtual ~PlantDB() {
    _assert_(true);
    if (omode_ != 0) close();
    if (!curs_.empty()) {
      typename CursorList::const_iterator cit = curs_.begin();
      typename CursorList::const_iterator citend = curs_.end();
      while (cit != citend) {
        Cursor* cur = *cit;
        cur->db_ = NULL;
        ++cit;
      }
    }
  }
  /**
   * Accept a visitor to a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @return true on success, or false on failure.
   * @note The operation for each record is performed atomically and other threads accessing the
   * same record are blocked.  To avoid deadlock, any explicit database operation must not be
   * performed in this function.
   */
  bool accept(const char* kbuf, size_t ksiz, Visitor* visitor, bool writable = true) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && visitor);
    bool wrlock = writable && (tran_ || autotran_);
    if (wrlock) {
      mlock_.lock_writer();
    } else {
      mlock_.lock_reader();
    }
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    if (writable && !writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      mlock_.unlock();
      return false;
    }
    char lstack[KCPDRECBUFSIZ];
    size_t lsiz = sizeof(Link) + ksiz;
    char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
    Link* link = (Link*)lbuf;
    link->child = 0;
    link->ksiz = ksiz;
    std::memcpy(lbuf + sizeof(*link), kbuf, ksiz);
    int64_t hist[LEVELMAX];
    int32_t hnum = 0;
    LeafNode* node = search_tree(link, true, hist, &hnum);
    if (!node) {
      set_error(_KCCODELINE_, Error::BROKEN, "search failed");
      if (lbuf != lstack) delete[] lbuf;
      mlock_.unlock();
      return false;
    }
    char rstack[KCPDRECBUFSIZ];
    size_t rsiz = sizeof(Record) + ksiz;
    char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
    Record* rec = (Record*)rbuf;
    rec->ksiz = ksiz;
    rec->vsiz = 0;
    std::memcpy(rbuf + sizeof(*rec), kbuf, ksiz);
    if (writable) {
      node->lock.lock_writer();
    } else {
      node->lock.lock_reader();
    }
    bool reorg = accept_impl(node, rec, visitor);
    bool atran = autotran_ && !tran_ && node->dirty;
    bool async = autosync_ && !autotran_ && !tran_ && node->dirty;
    node->lock.unlock();
    bool flush = false;
    bool err = false;
    int64_t id = node->id;
    if (atran && !reorg && !fix_auto_transaction_leaf(node)) err = true;
    if (cusage_ > pccap_) {
      int32_t idx = id % SLOTNUM;
      LeafSlot* lslot = lslots_ + idx;
      if (!clean_leaf_cache_part(lslot)) err = true;
      flush = true;
    }
    if (reorg) {
      if (!wrlock) {
        mlock_.unlock();
        mlock_.lock_writer();
      }
      node = search_tree(link, false, hist, &hnum);
      if (node) {
        if (!reorganize_tree(node, hist, hnum)) err = true;
        if (atran && !tran_ && !fix_auto_transaction_tree()) err = true;
      }
      mlock_.unlock();
    } else if (flush) {
      if (!wrlock) {
        mlock_.unlock();
        mlock_.lock_writer();
      }
      int32_t idx = id % SLOTNUM;
      LeafSlot* lslot = lslots_ + idx;
      if (!flush_leaf_cache_part(lslot)) err = true;
      InnerSlot* islot = islots_ + idx;
      if (islot->warm->count() > lslot->warm->count() + lslot->hot->count() + 1 &&
          !flush_inner_cache_part(islot)) err = true;
      mlock_.unlock();
    } else {
      mlock_.unlock();
    }
    if (rbuf != rstack) delete[] rbuf;
    if (lbuf != lstack) delete[] lbuf;
    if (async) {
      mlock_.lock_writer();
      if (!fix_auto_synchronization()) err = true;
      mlock_.unlock();
    }
    return !err;
  }
  /**
   * Accept a visitor to multiple records at once.
   * @param keys specifies a string vector of the keys.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @return true on success, or false on failure.
   * @note The operations for specified records are performed atomically and other threads
   * accessing the same records are blocked.  To avoid deadlock, any explicit database operation
   * must not be performed in this function.
   */
  bool accept_bulk(const std::vector<std::string>& keys, Visitor* visitor,
                   bool writable = true) {
    _assert_(visitor);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (writable && !writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    ScopedVisitor svis(visitor);
    if (keys.empty()) return true;
    bool err = false;
    std::vector<std::string>::const_iterator kit = keys.begin();
    std::vector<std::string>::const_iterator kitend = keys.end();
    while (!err && kit != kitend) {
      const char* kbuf = kit->data();
      size_t ksiz = kit->size();
      char lstack[KCPDRECBUFSIZ];
      size_t lsiz = sizeof(Link) + ksiz;
      char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
      Link* link = (Link*)lbuf;
      link->child = 0;
      link->ksiz = ksiz;
      std::memcpy(lbuf + sizeof(*link), kbuf, ksiz);
      int64_t hist[LEVELMAX];
      int32_t hnum = 0;
      LeafNode* node = search_tree(link, true, hist, &hnum);
      if (!node) {
        set_error(_KCCODELINE_, Error::BROKEN, "search failed");
        if (lbuf != lstack) delete[] lbuf;
        err = true;
        break;
      }
      char rstack[KCPDRECBUFSIZ];
      size_t rsiz = sizeof(Record) + ksiz;
      char* rbuf = rsiz > sizeof(rstack) ? new char[rsiz] : rstack;
      Record* rec = (Record*)rbuf;
      rec->ksiz = ksiz;
      rec->vsiz = 0;
      std::memcpy(rbuf + sizeof(*rec), kbuf, ksiz);
      bool reorg = accept_impl(node, rec, visitor);
      bool atran = autotran_ && !tran_ && node->dirty;
      bool async = autosync_ && !autotran_ && !tran_ && node->dirty;
      if (atran && !reorg && !fix_auto_transaction_leaf(node)) err = true;
      if (reorg) {
        if (!reorganize_tree(node, hist, hnum)) err = true;
        if (atran && !fix_auto_transaction_tree()) err = true;
      } else if (cusage_ > pccap_) {
        int32_t idx = node->id % SLOTNUM;
        LeafSlot* lslot = lslots_ + idx;
        if (!flush_leaf_cache_part(lslot)) err = true;
        InnerSlot* islot = islots_ + idx;
        if (islot->warm->count() > lslot->warm->count() + lslot->hot->count() + 1 &&
            !flush_inner_cache_part(islot)) err = true;
      }
      if (rbuf != rstack) delete[] rbuf;
      if (lbuf != lstack) delete[] lbuf;
      if (async && !fix_auto_synchronization()) err = true;
      ++kit;
    }
    return !err;
  }
  /**
   * Iterate to accept a visitor for each record.
   * @param visitor a visitor object.
   * @param writable true for writable operation, or false for read-only operation.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note The whole iteration is performed atomically and other threads are blocked.  To avoid
   * deadlock, any explicit database operation must not be performed in this function.
   */
  bool iterate(Visitor *visitor, bool writable = true, ProgressChecker* checker = NULL) {
    _assert_(visitor);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (writable && !writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    ScopedVisitor svis(visitor);
    int64_t allcnt = count_;
    if (checker && !checker->check("iterate", "beginning", 0, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    bool err = false;
    bool atran = false;
    if (autotran_ && writable && !tran_) {
      if (begin_transaction_impl(autosync_)) {
        atran = true;
      } else {
        err = true;
      }
    }
    int64_t id = first_;
    int64_t flcnt = 0;
    int64_t curcnt = 0;
    while (!err && id > 0) {
      LeafNode* node = load_leaf_node(id, false);
      if (!node) {
        set_error(_KCCODELINE_, Error::BROKEN, "missing leaf node");
        db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)id);
        return false;
      }
      id = node->next;
      const RecordArray& recs = node->recs;
      RecordArray keys;
      keys.reserve(recs.size());
      typename RecordArray::const_iterator rit = recs.begin();
      typename RecordArray::const_iterator ritend = recs.end();
      while (rit != ritend) {
        Record* rec = *rit;
        size_t rsiz = sizeof(*rec) + rec->ksiz;
        char* dbuf = (char*)rec + sizeof(*rec);
        Record* key = (Record*)xmalloc(rsiz);
        key->ksiz = rec->ksiz;
        key->vsiz = 0;
        char* kbuf = (char*)key + sizeof(*key);
        std::memcpy(kbuf, dbuf, rec->ksiz);
        keys.push_back(key);
        ++rit;
      }
      typename RecordArray::const_iterator kit = keys.begin();
      typename RecordArray::const_iterator kitend = keys.end();
      bool reorg = false;
      while (kit != kitend) {
        Record* rec = *kit;
        if (accept_impl(node, rec, visitor)) reorg = true;
        curcnt++;
        if (checker && !checker->check("iterate", "processing", curcnt, allcnt)) {
          set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
          err = true;
          break;
        }
        ++kit;
      }
      if (reorg) {
        Record* rec = keys.front();
        char* dbuf = (char*)rec + sizeof(*rec);
        char lstack[KCPDRECBUFSIZ];
        size_t lsiz = sizeof(Link) + rec->ksiz;
        char* lbuf = lsiz > sizeof(lstack) ? new char[lsiz] : lstack;
        Link* link = (Link*)lbuf;
        link->child = 0;
        link->ksiz = rec->ksiz;
        std::memcpy(lbuf + sizeof(*link), dbuf, rec->ksiz);
        int64_t hist[LEVELMAX];
        int32_t hnum = 0;
        node = search_tree(link, false, hist, &hnum);
        if (node) {
          if (!reorganize_tree(node, hist, hnum)) err = true;
        } else {
          set_error(_KCCODELINE_, Error::BROKEN, "search failed");
          err = true;
        }
        if (lbuf != lstack) delete[] lbuf;
      }
      if (cusage_ > pccap_) {
        for (int32_t i = 0; i < SLOTNUM; i++) {
          LeafSlot* lslot = lslots_ + i;
          if (!flush_leaf_cache_part(lslot)) err = true;
        }
        InnerSlot* islot = islots_ + (flcnt++) % SLOTNUM;
        if (islot->warm->count() > 2 && !flush_inner_cache_part(islot)) err = true;
      }
      kit = keys.begin();
      while (kit != kitend) {
        xfree(*kit);
        ++kit;
      }
    }
    if (checker && !checker->check("iterate", "ending", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      err = true;
    }
    if (atran && !commit_transaction()) err = true;
    if (autosync_ && !autotran_ && writable && !fix_auto_synchronization()) err = true;
    trigger_meta(MetaTrigger::ITERATE, "iterate");
    return !err;
  }
  /**
   * Scan each record in parallel.
   * @param visitor a visitor object.
   * @param thnum the number of worker threads.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note This function is for reading records and not for updating ones.  The return value of
   * the visitor is just ignored.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  bool scan_parallel(Visitor *visitor, size_t thnum, ProgressChecker* checker = NULL) {
    _assert_(visitor && thnum <= MEMMAXSIZ);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (thnum < 1) thnum = 0;
    if (thnum > (size_t)INT8MAX) thnum = INT8MAX;
    bool err = false;
    if (writer_) {
      if (checker && !checker->check("scan_parallel", "cleaning the leaf node cache", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (!clean_leaf_cache()) err = true;
    }
    ScopedVisitor svis(visitor);
    int64_t allcnt = count_;
    if (checker && !checker->check("scan_parallel", "beginning", 0, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    class ProgressCheckerImpl : public ProgressChecker {
     public:
      explicit ProgressCheckerImpl() : ok_(1) {}
      void stop() {
        ok_.set(0);
      }
     private:
      bool check(const char* name, const char* message, int64_t curcnt, int64_t allcnt) {
        return ok_ > 0;
      }
      AtomicInt64 ok_;
    };
    ProgressCheckerImpl ichecker;
    class VisitorImpl : public Visitor {
     public:
      explicit VisitorImpl(PlantDB* db, Visitor* visitor,
                           ProgressChecker* checker, int64_t allcnt,
                           ProgressCheckerImpl* ichecker) :
          db_(db), visitor_(visitor), checker_(checker), allcnt_(allcnt),
          ichecker_(ichecker), error_() {}
      const Error& error() {
        return error_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 2 || ksiz >= NUMBUFSIZ || kbuf[0] != LNPREFIX) return NOP;
        uint64_t prev;
        size_t step = readvarnum(vbuf, vsiz, &prev);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        uint64_t next;
        step = readvarnum(vbuf, vsiz, &next);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        while (vsiz > 1) {
          uint64_t rksiz;
          step = readvarnum(vbuf, vsiz, &rksiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          uint64_t rvsiz;
          step = readvarnum(vbuf, vsiz, &rvsiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          if (vsiz < rksiz + rvsiz) break;
          size_t xvsiz;
          visitor_->visit_full(vbuf, rksiz, vbuf + rksiz, rvsiz, &xvsiz);
          vbuf += rksiz;
          vsiz -= rksiz;
          vbuf += rvsiz;
          vsiz -= rvsiz;
          if (checker_ && !checker_->check("scan_parallel", "processing", -1, allcnt_)) {
            db_->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
            error_ = db_->error();
            ichecker_->stop();
            break;
          }
        }
        return NOP;
      }
      PlantDB* db_;
      Visitor* visitor_;
      ProgressChecker* checker_;
      int64_t allcnt_;
      ProgressCheckerImpl* ichecker_;
      Error error_;
    };
    VisitorImpl ivisitor(this, visitor, checker, allcnt, &ichecker);
    if (!db_.scan_parallel(&ivisitor, thnum, &ichecker)) err = true;
    if (ivisitor.error() != Error::SUCCESS) {
      const Error& e = ivisitor.error();
      db_.set_error(_KCCODELINE_, e.code(), e.message());
      err = true;
    }
    if (checker && !checker->check("scan_parallel", "ending", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      err = true;
    }
    trigger_meta(MetaTrigger::ITERATE, "scan_parallel");
    return !err;
  }
  /**
   * Get the last happened error.
   * @return the last happened error.
   */
  Error error() const {
    _assert_(true);
    return db_.error();
  }
  /**
   * Set the error information.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param code an error code.
   * @param message a supplement message.
   */
  void set_error(const char* file, int32_t line, const char* func,
                 Error::Code code, const char* message) {
    _assert_(file && line > 0 && func && message);
    db_.set_error(file, line, func, code, message);
  }
  /**
   * Open a database file.
   * @param path the path of a database file.
   * @param mode the connection mode.  BasicDB::OWRITER as a writer, BasicDB::OREADER as a
   * reader.  The following may be added to the writer mode by bitwise-or: BasicDB::OCREATE,
   * which means it creates a new database if the file does not exist, BasicDB::OTRUNCATE, which
   * means it creates a new database regardless if the file exists, BasicDB::OAUTOTRAN, which
   * means each updating operation is performed in implicit transaction, BasicDB::OAUTOSYNC,
   * which means each updating operation is followed by implicit synchronization with the file
   * system.  The following may be added to both of the reader mode and the writer mode by
   * bitwise-or: BasicDB::ONOLOCK, which means it opens the database file without file locking,
   * BasicDB::OTRYLOCK, which means locking is performed without blocking, BasicDB::ONOREPAIR,
   * which means the database file is not repaired implicitly even if file destruction is
   * detected.
   * @return true on success, or false on failure.
   * @note Every opened database must be closed by the BasicDB::close method when it is no
   * longer in use.  It is not allowed for two or more database objects in the same process to
   * keep their connections to the same database file at the same time.
   */
  bool open(const std::string& path, uint32_t mode = OWRITER | OCREATE) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    report(_KCCODELINE_, Logger::DEBUG, "opening the database (path=%s)", path.c_str());
    if (DBTYPE == TYPEGRASS) {
      mode &= ~OREADER;
      mode |= OWRITER | OCREATE;
    }
    writer_ = false;
    autotran_ = false;
    autosync_ = false;
    if (mode & OWRITER) {
      writer_ = true;
      if (mode & OAUTOTRAN) autotran_ = true;
      if (mode & OAUTOSYNC) autosync_ = true;
    }
    if (!db_.tune_type(DBTYPE)) return false;
    if (!db_.tune_alignment(apow_)) return false;
    if (!db_.tune_fbp(fpow_)) return false;
    if (!db_.tune_options(opts_)) return false;
    if (!db_.tune_buckets(bnum_)) return false;
    if (!db_.open(path, mode)) return false;
    if (db_.type() != DBTYPE) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid database type");
      db_.close();
      return false;
    }
    if (db_.reorganized()) {
      if (!reorganize_file(mode)) return false;
    } else if (db_.recovered()) {
      if (!writer_) {
        if (!db_.close()) return false;
        uint32_t tmode = (mode & ~OREADER) | OWRITER;
        if (!db_.open(path, tmode)) return false;
      }
      if (!recalc_count()) return false;
      if (!writer_) {
        if (!db_.close()) return false;
        if (!db_.open(path, mode)) return false;
      }
      if (count_ == INT64MAX && !reorganize_file(mode)) return false;
    }
    if (writer_ && db_.count() < 1) {
      root_ = 0;
      first_ = 0;
      last_ = 0;
      count_ = 0;
      create_leaf_cache();
      create_inner_cache();
      lcnt_ = 0;
      create_leaf_node(0, 0);
      root_ = 1;
      first_ = 1;
      last_ = 1;
      lcnt_ = 1;
      icnt_ = 0;
      count_ = 0;
      if (!reccomp_.comp) reccomp_.comp = LEXICALCOMP;
      if (!dump_meta() || !flush_leaf_cache(true) || !load_meta()) {
        delete_inner_cache();
        delete_leaf_cache();
        db_.close();
        return false;
      }
    } else {
      if (!load_meta()) {
        db_.close();
        return false;
      }
      create_leaf_cache();
      create_inner_cache();
    }
    if (psiz_ < 1 || root_ < 1 || first_ < 1 || last_ < 1 ||
        lcnt_ < 1 || icnt_ < 0 || count_ < 0 || bnum_ < 1) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid meta data");
      db_.report(_KCCODELINE_, Logger::WARN, "psiz=%lld root=%lld first=%lld last=%lld"
                 " lcnt=%lld icnt=%lld count=%lld bnum=%lld",
                 (long long)psiz_, (long long)root_, (long long)first_, (long long)last_,
                 (long long)lcnt_, (long long)icnt_, (long long)count_, (long long)bnum_);
      delete_inner_cache();
      delete_leaf_cache();
      db_.close();
      return false;
    }
    omode_ = mode;
    cusage_ = 0;
    tran_ = false;
    trclock_ = 0;
    trigger_meta(MetaTrigger::OPEN, "open");
    return true;
  }
  /**
   * Close the database file.
   * @return true on success, or false on failure.
   */
  bool close() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    const std::string& path = db_.path();
    report(_KCCODELINE_, Logger::DEBUG, "closing the database (path=%s)", path.c_str());
    bool err = false;
    disable_cursors();
    int64_t lsiz = calc_leaf_cache_size();
    int64_t isiz = calc_inner_cache_size();
    if (cusage_ != lsiz + isiz) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid cache usage");
      db_.report(_KCCODELINE_, Logger::WARN, "cusage=%lld lsiz=%lld isiz=%lld",
                 (long long)cusage_, (long long)lsiz, (long long)isiz);
      err = true;
    }
    if (!flush_leaf_cache(true)) err = true;
    if (!flush_inner_cache(true)) err = true;
    lsiz = calc_leaf_cache_size();
    isiz = calc_inner_cache_size();
    int64_t lcnt = calc_leaf_cache_count();
    int64_t icnt = calc_inner_cache_count();
    if (cusage_ != 0 || lsiz != 0 || isiz != 0 || lcnt != 0 || icnt != 0) {
      set_error(_KCCODELINE_, Error::BROKEN, "remaining cache");
      db_.report(_KCCODELINE_, Logger::WARN, "cusage=%lld lsiz=%lld isiz=%lld"
                 " lcnt=%lld icnt=%lld", (long long)cusage_, (long long)lsiz, (long long)isiz,
                 (long long)lcnt, (long long)icnt);
      err = true;
    }
    delete_inner_cache();
    delete_leaf_cache();
    if (writer_ && !dump_meta()) err = true;
    if (!db_.close()) err = true;
    omode_ = 0;
    trigger_meta(MetaTrigger::CLOSE, "close");
    return !err;
  }
  /**
   * Synchronize updated contents with the file and the device.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @param proc a postprocessor object.  If it is NULL, no postprocessing is performed.
   * @param checker a progress checker object.  If it is NULL, no checking is performed.
   * @return true on success, or false on failure.
   * @note The operation of the postprocessor is performed atomically and other threads accessing
   * the same record are blocked.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  bool synchronize(bool hard = false, FileProcessor* proc = NULL,
                   ProgressChecker* checker = NULL) {
    _assert_(true);
    mlock_.lock_reader();
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    bool err = false;
    if (writer_) {
      if (checker && !checker->check("synchronize", "cleaning the leaf node cache", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        mlock_.unlock();
        return false;
      }
      if (!clean_leaf_cache()) err = true;
      if (checker && !checker->check("synchronize", "cleaning the inner node cache", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        mlock_.unlock();
        return false;
      }
      if (!clean_inner_cache()) err = true;
      mlock_.unlock();
      mlock_.lock_writer();
      if (checker && !checker->check("synchronize", "flushing the leaf node cache", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        mlock_.unlock();
        return false;
      }
      if (!flush_leaf_cache(true)) err = true;
      if (checker && !checker->check("synchronize", "flushing the inner node cache", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        mlock_.unlock();
        return false;
      }
      if (!flush_inner_cache(true)) err = true;
      if (checker && !checker->check("synchronize", "dumping the meta data", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        mlock_.unlock();
        return false;
      }
      if (!dump_meta()) err = true;
    }
    class Wrapper : public FileProcessor {
     public:
      Wrapper(FileProcessor* proc, int64_t count) : proc_(proc), count_(count) {}
     private:
      bool process(const std::string& path, int64_t count, int64_t size) {
        if (proc_) return proc_->process(path, count_, size);
        return true;
      }
      FileProcessor* proc_;
      int64_t count_;
    } wrapper(proc, count_);
    if (!db_.synchronize(hard, &wrapper, checker)) err = true;
    trigger_meta(MetaTrigger::SYNCHRONIZE, "synchronize");
    mlock_.unlock();
    return !err;
  }
  /**
   * Occupy database by locking and do something meanwhile.
   * @param writable true to use writer lock, or false to use reader lock.
   * @param proc a processor object.  If it is NULL, no processing is performed.
   * @return true on success, or false on failure.
   * @note The operation of the processor is performed atomically and other threads accessing
   * the same record are blocked.  To avoid deadlock, any explicit database operation must not
   * be performed in this function.
   */
  bool occupy(bool writable = true, FileProcessor* proc = NULL) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, writable);
    bool err = false;
    if (proc && !proc->process(db_.path(), count_, db_.size())) {
      set_error(_KCCODELINE_, Error::LOGIC, "processing failed");
      err = true;
    }
    trigger_meta(MetaTrigger::OCCUPY, "occupy");
    return !err;
  }
  /**
   * Begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool begin_transaction(bool hard = false) {
    _assert_(true);
    uint32_t wcnt = 0;
    while (true) {
      mlock_.lock_writer();
      if (omode_ == 0) {
        set_error(_KCCODELINE_, Error::INVALID, "not opened");
        mlock_.unlock();
        return false;
      }
      if (!writer_) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        mlock_.unlock();
        return false;
      }
      if (!tran_) break;
      mlock_.unlock();
      if (wcnt >= LOCKBUSYLOOP) {
        Thread::chill();
      } else {
        Thread::yield();
        wcnt++;
      }
    }
    if (!begin_transaction_impl(hard)) {
      mlock_.unlock();
      return false;
    }
    tran_ = true;
    trigger_meta(MetaTrigger::BEGINTRAN, "begin_transaction");
    mlock_.unlock();
    return true;
  }
  /**
   * Try to begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool begin_transaction_try(bool hard = false) {
    _assert_(true);
    mlock_.lock_writer();
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    if (!writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      mlock_.unlock();
      return false;
    }
    if (tran_) {
      set_error(_KCCODELINE_, Error::LOGIC, "competition avoided");
      mlock_.unlock();
      return false;
    }
    if (!begin_transaction_impl(hard)) {
      mlock_.unlock();
      return false;
    }
    tran_ = true;
    trigger_meta(MetaTrigger::BEGINTRAN, "begin_transaction_try");
    mlock_.unlock();
    return true;
  }
  /**
   * End transaction.
   * @param commit true to commit the transaction, or false to abort the transaction.
   * @return true on success, or false on failure.
   */
  bool end_transaction(bool commit = true) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (!tran_) {
      set_error(_KCCODELINE_, Error::INVALID, "not in transaction");
      return false;
    }
    bool err = false;
    if (commit) {
      if (!commit_transaction()) err = true;
    } else {
      if (!abort_transaction()) err = true;
    }
    tran_ = false;
    trigger_meta(commit ? MetaTrigger::COMMITTRAN : MetaTrigger::ABORTTRAN, "end_transaction");
    return !err;
  }
  /**
   * Remove all records.
   * @return true on success, or false on failure.
   */
  bool clear() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (!writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    disable_cursors();
    flush_leaf_cache(false);
    flush_inner_cache(false);
    bool err = false;
    if (!db_.clear()) err = true;
    lcnt_ = 0;
    create_leaf_node(0, 0);
    root_ = 1;
    first_ = 1;
    last_ = 1;
    lcnt_ = 1;
    icnt_ = 0;
    count_ = 0;
    if (!dump_meta()) err = true;
    if (!flush_leaf_cache(true)) err = true;
    cusage_ = 0;
    trigger_meta(MetaTrigger::CLEAR, "clear");
    return !err;
  }
  /**
   * Get the number of records.
   * @return the number of records, or -1 on failure.
   */
  int64_t count() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return -1;
    }
    return count_;
  }
  /**
   * Get the size of the database file.
   * @return the size of the database file in bytes, or -1 on failure.
   */
  int64_t size() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return -1;
    }
    return db_.size();
  }
  /**
   * Get the path of the database file.
   * @return the path of the database file, or an empty string on failure.
   */
  std::string path() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return "";
    }
    return db_.path();
  }
  /**
   * Get the miscellaneous status information.
   * @param strmap a string map to contain the result.
   * @return true on success, or false on failure.
   */
  bool status(std::map<std::string, std::string>* strmap) {
    _assert_(strmap);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (!db_.status(strmap)) return false;
    (*strmap)["type"] = strprintf("%u", (unsigned)DBTYPE);
    (*strmap)["psiz"] = strprintf("%d", psiz_);
    (*strmap)["pccap"] = strprintf("%lld", (long long)pccap_);
    const char* compname = "external";
    if (reccomp_.comp == LEXICALCOMP) {
      compname = "lexical";
    } else if (reccomp_.comp == DECIMALCOMP) {
      compname = "decimal";
    } else if (reccomp_.comp == LEXICALDESCCOMP) {
      compname = "lexicaldesc";
    } else if (reccomp_.comp == DECIMALDESCCOMP) {
      compname = "decimaldesc";
    }
    (*strmap)["rcomp"] = compname;
    (*strmap)["root"] = strprintf("%lld", (long long)root_);
    (*strmap)["first"] = strprintf("%lld", (long long)first_);
    (*strmap)["last"] = strprintf("%lld", (long long)last_);
    (*strmap)["lcnt"] = strprintf("%lld", (long long)lcnt_);
    (*strmap)["icnt"] = strprintf("%lld", (long long)icnt_);
    (*strmap)["count"] = strprintf("%lld", (long long)count_);
    (*strmap)["bnum"] = strprintf("%lld", (long long)bnum_);
    (*strmap)["pnum"] = strprintf("%lld", (long long)db_.count());
    (*strmap)["cusage"] = strprintf("%lld", (long long)cusage_);
    if (strmap->count("cusage_lcnt") > 0)
      (*strmap)["cusage_lcnt"] = strprintf("%lld", (long long)calc_leaf_cache_count());
    if (strmap->count("cusage_lsiz") > 0)
      (*strmap)["cusage_lsiz"] = strprintf("%lld", (long long)calc_leaf_cache_size());
    if (strmap->count("cusage_icnt") > 0)
      (*strmap)["cusage_icnt"] = strprintf("%lld", (long long)calc_inner_cache_count());
    if (strmap->count("cusage_isiz") > 0)
      (*strmap)["cusage_isiz"] = strprintf("%lld", (long long)calc_inner_cache_size());
    if (strmap->count("tree_level") > 0) {
      Link link;
      link.ksiz = 0;
      int64_t hist[LEVELMAX];
      int32_t hnum = 0;
      search_tree(&link, false, hist, &hnum);
      (*strmap)["tree_level"] = strprintf("%d", hnum + 1);
    }
    return true;
  }
  /**
   * Create a cursor object.
   * @return the return value is the created cursor object.
   * @note Because the object of the return value is allocated by the constructor, it should be
   * released with the delete operator when it is no longer in use.
   */
  Cursor* cursor() {
    _assert_(true);
    return new Cursor(this);
  }
  /**
   * Write a log message.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param message the supplement message.
   */
  void log(const char* file, int32_t line, const char* func, Logger::Kind kind,
           const char* message) {
    _assert_(file && line > 0 && func && message);
    ScopedRWLock lock(&mlock_, false);
    db_.log(file, line, func, kind, message);
  }
  /**
   * Set the internal logger.
   * @param logger the logger object.
   * @param kinds kinds of logged messages by bitwise-or: Logger::DEBUG for debugging,
   * Logger::INFO for normal information, Logger::WARN for warning, and Logger::ERROR for fatal
   * error.
   * @return true on success, or false on failure.
   */
  bool tune_logger(Logger* logger, uint32_t kinds = Logger::WARN | Logger::ERROR) {
    _assert_(logger);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    return db_.tune_logger(logger, kinds);
  }
  /**
   * Set the internal meta operation trigger.
   * @param trigger the trigger object.
   * @return true on success, or false on failure.
   */
  bool tune_meta_trigger(MetaTrigger* trigger) {
    _assert_(trigger);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    mtrigger_ = trigger;
    return true;
  }
  /**
   * Set the power of the alignment of record size.
   * @param apow the power of the alignment of record size.
   * @return true on success, or false on failure.
   */
  bool tune_alignment(int8_t apow) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    apow_ = apow >= 0 ? apow : DEFAPOW;
    return true;
  }
  /**
   * Set the power of the capacity of the free block pool.
   * @param fpow the power of the capacity of the free block pool.
   * @return true on success, or false on failure.
   */
  bool tune_fbp(int8_t fpow) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    fpow_ = fpow >= 0 ? fpow : DEFFPOW;
    return true;
  }
  /**
   * Set the optional features.
   * @param opts the optional features by bitwise-or: BasicDB::TSMALL to use 32-bit addressing,
   * BasicDB::TLINEAR to use linear collision chaining, BasicDB::TCOMPRESS to compress each
   * record.
   * @return true on success, or false on failure.
   */
  bool tune_options(int8_t opts) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    opts_ = opts;
    return true;
  }
  /**
   * Set the number of buckets of the hash table.
   * @param bnum the number of buckets of the hash table.
   * @return true on success, or false on failure.
   */
  bool tune_buckets(int64_t bnum) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    bnum_ = bnum > 0 ? bnum : DEFBNUM;
    return true;
  }
  /**
   * Set the size of each page.
   * @param psiz the size of each page.
   * @return true on success, or false on failure.
   */
  bool tune_page(int32_t psiz) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    psiz_ = psiz > 0 ? psiz : DEFPSIZ;
    return true;
  }
  /**
   * Set the size of the internal memory-mapped region.
   * @param msiz the size of the internal memory-mapped region.
   * @return true on success, or false on failure.
   */
  bool tune_map(int64_t msiz) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    return db_.tune_map(msiz);
  }
  /**
   * Set the unit step number of auto defragmentation.
   * @param dfunit the unit step number of auto defragmentation.
   * @return true on success, or false on failure.
   */
  bool tune_defrag(int64_t dfunit) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    return db_.tune_defrag(dfunit);
  }
  /**
   * Set the capacity size of the page cache.
   * @param pccap the capacity size of the page cache.
   * @return true on success, or false on failure.
   */
  bool tune_page_cache(int64_t pccap) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    pccap_ = pccap > 0 ? pccap : DEFPCCAP;
    return true;
  }
  /**
   * Set the data compressor.
   * @param comp the data compressor object.
   * @return true on success, or false on failure.
   */
  bool tune_compressor(Compressor* comp) {
    _assert_(comp);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    return db_.tune_compressor(comp);
  }
  /**
   * Set the record comparator.
   * @param rcomp the record comparator object.
   * @return true on success, or false on failure.
   * @note Several built-in comparators are provided.  LEXICALCOMP for the default lexical
   * comparator.  DECIMALCOMP for the decimal comparator.  LEXICALDESCCOMP for the lexical
   * descending comparator.  DECIMALDESCCOMP for the lexical descending comparator.
   */
  bool tune_comparator(Comparator* rcomp) {
    _assert_(rcomp);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    reccomp_.comp = rcomp;
    return true;
  }
  /**
   * Get the opaque data.
   * @return the pointer to the opaque data region, whose size is 16 bytes.
   */
  char* opaque() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return NULL;
    }
    return db_.opaque();
  }
  /**
   * Synchronize the opaque data.
   * @return true on success, or false on failure.
   */
  bool synchronize_opaque() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    return db_.synchronize_opaque();
  }
  /**
   * Perform defragmentation of the file.
   * @param step the number of steps.  If it is not more than 0, the whole region is defraged.
   * @return true on success, or false on failure.
   */
  bool defrag(int64_t step = 0) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    bool err = false;
    if (step < 1 && writer_) {
      if (!clean_leaf_cache()) err = true;
      if (!clean_inner_cache()) err = true;
    }
    if (!db_.defrag(step)) err = true;
    return !err;
  }
  /**
   * Get the status flags.
   * @return the status flags, or 0 on failure.
   */
  uint8_t flags() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return db_.flags();
  }
  /**
   * Get the record comparator.
   * @return the record comparator object.
   */
  Comparator* rcomp() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return reccomp_.comp;
  }
 protected:
  /**
   * Report a message for debugging.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param format the printf-like format string.
   * @param ... used according to the format string.
   */
  void report(const char* file, int32_t line, const char* func, Logger::Kind kind,
              const char* format, ...) {
    _assert_(file && line > 0 && func && format);
    va_list ap;
    va_start(ap, format);
    db_.report_valist(file, line, func, kind, format, ap);
    va_end(ap);
  }
  /**
   * Report a message for debugging with variable number of arguments.
   * @param file the file name of the program source code.
   * @param line the line number of the program source code.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param format the printf-like format string.
   * @param ap used according to the format string.
   */
  void report_valist(const char* file, int32_t line, const char* func, Logger::Kind kind,
                     const char* format, va_list ap) {
    _assert_(file && line > 0 && func && format);
    db_.report_valist(file, line, func, kind, format, ap);
  }
  /**
   * Report the content of a binary buffer for debugging.
   * @param file the file name of the epicenter.
   * @param line the line number of the epicenter.
   * @param func the function name of the program source code.
   * @param kind the kind of the event.  Logger::DEBUG for debugging, Logger::INFO for normal
   * information, Logger::WARN for warning, and Logger::ERROR for fatal error.
   * @param name the name of the information.
   * @param buf the binary buffer.
   * @param size the size of the binary buffer
   */
  void report_binary(const char* file, int32_t line, const char* func, Logger::Kind kind,
                     const char* name, const char* buf, size_t size) {
    _assert_(file && line > 0 && func && name && buf && size <= MEMMAXSIZ);
    db_.report_binary(file, line, func, kind, name, buf, size);
  }
  /**
   * Trigger a meta database operation.
   * @param kind the kind of the event.  MetaTrigger::OPEN for opening, MetaTrigger::CLOSE for
   * closing, MetaTrigger::CLEAR for clearing, MetaTrigger::ITERATE for iteration,
   * MetaTrigger::SYNCHRONIZE for synchronization, MetaTrigger::BEGINTRAN for beginning
   * transaction, MetaTrigger::COMMITTRAN for committing transaction, MetaTrigger::ABORTTRAN
   * for aborting transaction, and MetaTrigger::MISC for miscellaneous operations.
   * @param message the supplement message.
   */
  void trigger_meta(MetaTrigger::Kind kind, const char* message) {
    _assert_(message);
    if (mtrigger_) mtrigger_->trigger(kind, message);
  }
 private:
  /**
   * Record data.
   */
  struct Record {
    uint32_t ksiz;                       ///< size of the key
    uint32_t vsiz;                       ///< size of the value
  };
  /**
   * Comparator for records.
   */
  struct RecordComparator {
    Comparator* comp;                    ///< comparator
    /** constructor */
    explicit RecordComparator() : comp(NULL) {}
    /** comparing operator */
    bool operator ()(const Record* const& a, const Record* const& b) const {
      _assert_(true);
      char* akbuf = (char*)a + sizeof(*a);
      char* bkbuf = (char*)b + sizeof(*b);
      return comp->compare(akbuf, a->ksiz, bkbuf, b->ksiz) < 0;
    }
  };
  /**
   * Leaf node of B+ tree.
   */
  struct LeafNode {
    RWLock lock;                         ///< lock
    int64_t id;                          ///< page ID number
    RecordArray recs;                    ///< sorted array of records
    int64_t size;                        ///< total size of records
    int64_t prev;                        ///< previous leaf node
    int64_t next;                        ///< next leaf node
    bool hot;                            ///< whether in the hot cache
    bool dirty;                          ///< whether to be written back
    bool dead;                           ///< whether to be removed
  };
  /**
   * Link to a node.
   */
  struct Link {
    int64_t child;                       ///< child node
    int32_t ksiz;                        ///< size of the key
  };
  /**
   * Comparator for links.
   */
  struct LinkComparator {
    Comparator* comp;                    ///< comparator
    /** constructor */
    explicit LinkComparator() : comp(NULL) {
      _assert_(true);
    }
    /** comparing operator */
    bool operator ()(const Link* const& a, const Link* const& b) const {
      _assert_(true);
      char* akbuf = (char*)a + sizeof(*a);
      char* bkbuf = (char*)b + sizeof(*b);
      return comp->compare(akbuf, a->ksiz, bkbuf, b->ksiz) < 0;
    }
  };
  /**
   * Inner node of B+ tree.
   */
  struct InnerNode {
    RWLock lock;                         ///< lock
    int64_t id;                          ///< page ID numger
    int64_t heir;                        ///< child before the first link
    LinkArray links;                     ///< sorted array of links
    int64_t size;                        ///< total size of links
    bool dirty;                          ///< whether to be written back
    bool dead;                           ///< whether to be removed
  };
  /**
   * Slot cache of leaf nodes.
   */
  struct LeafSlot {
    Mutex lock;                          ///< lock
    LeafCache* hot;                      ///< hot cache
    LeafCache* warm;                     ///< warm cache
  };
  /**
   * Slot cache of inner nodes.
   */
  struct InnerSlot {
    Mutex lock;                          ///< lock
    InnerCache* warm;                    ///< warm cache
  };
  /**
   * Scoped visitor.
   */
  class ScopedVisitor {
   public:
    /** constructor */
    explicit ScopedVisitor(Visitor* visitor) : visitor_(visitor) {
      _assert_(visitor);
      visitor_->visit_before();
    }
    /** destructor */
    ~ScopedVisitor() {
      _assert_(true);
      visitor_->visit_after();
    }
   private:
    Visitor* visitor_;                   ///< visitor
  };
  /**
   * Open the leaf cache.
   */
  void create_leaf_cache() {
    _assert_(true);
    int64_t bnum = bnum_ / SLOTNUM + 1;
    if (bnum < INT8MAX) bnum = INT8MAX;
    bnum = nearbyprime(bnum);
    for (int32_t i = 0; i < SLOTNUM; i++) {
      lslots_[i].hot = new LeafCache(bnum);
      lslots_[i].warm = new LeafCache(bnum);
    }
  }
  /**
   * Close the leaf cache.
   */
  void delete_leaf_cache() {
    _assert_(true);
    for (int32_t i = SLOTNUM - 1; i >= 0; i--) {
      LeafSlot* slot = lslots_ + i;
      delete slot->warm;
      delete slot->hot;
    }
  }
  /**
   * Remove all leaf nodes from the leaf cache.
   * @param save whether to save dirty nodes.
   * @return true on success, or false on failure.
   */
  bool flush_leaf_cache(bool save) {
    _assert_(true);
    bool err = false;
    for (int32_t i = SLOTNUM - 1; i >= 0; i--) {
      LeafSlot* slot = lslots_ + i;
      typename LeafCache::Iterator it = slot->warm->begin();
      typename LeafCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        LeafNode* node = it.value();
        ++it;
        if (!flush_leaf_node(node, save)) err = true;
      }
      it = slot->hot->begin();
      itend = slot->hot->end();
      while (it != itend) {
        LeafNode* node = it.value();
        ++it;
        if (!flush_leaf_node(node, save)) err = true;
      }
    }
    return !err;
  }
  /**
   * Flush a part of the leaf cache.
   * @param slot a slot of leaf nodes.
   * @return true on success, or false on failure.
   */
  bool flush_leaf_cache_part(LeafSlot* slot) {
    _assert_(slot);
    bool err = false;
    if (slot->warm->count() > 0) {
      LeafNode* node = slot->warm->first_value();
      if (!flush_leaf_node(node, true)) err = true;
    } else if (slot->hot->count() > 0) {
      LeafNode* node = slot->hot->first_value();
      if (!flush_leaf_node(node, true)) err = true;
    }
    return !err;
  }
  /**
   * Clean all of the leaf cache.
   * @return true on success, or false on failure.
   */
  bool clean_leaf_cache() {
    _assert_(true);
    bool err = false;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      LeafSlot* slot = lslots_ + i;
      ScopedMutex lock(&slot->lock);
      typename LeafCache::Iterator it = slot->warm->begin();
      typename LeafCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        LeafNode* node = it.value();
        if (!save_leaf_node(node)) err = true;
        ++it;
      }
      it = slot->hot->begin();
      itend = slot->hot->end();
      while (it != itend) {
        LeafNode* node = it.value();
        if (!save_leaf_node(node)) err = true;
        ++it;
      }
    }
    return !err;
  }
  /**
   * Clean a part of the leaf cache.
   * @param slot a slot of leaf nodes.
   * @return true on success, or false on failure.
   */
  bool clean_leaf_cache_part(LeafSlot* slot) {
    _assert_(slot);
    bool err = false;
    ScopedMutex lock(&slot->lock);
    if (slot->warm->count() > 0) {
      LeafNode* node = slot->warm->first_value();
      if (!save_leaf_node(node)) err = true;
    } else if (slot->hot->count() > 0) {
      LeafNode* node = slot->hot->first_value();
      if (!save_leaf_node(node)) err = true;
    }
    return !err;
  }
  /**
   * Create a new leaf node.
   * @param prev the ID of the previous node.
   * @param next the ID of the next node.
   * @return the created leaf node.
   */
  LeafNode* create_leaf_node(int64_t prev, int64_t next) {
    _assert_(true);
    LeafNode* node = new LeafNode;
    node->id = ++lcnt_;
    node->size = sizeof(int32_t) * 2;
    node->recs.reserve(DEFLINUM);
    node->prev = prev;
    node->next = next;
    node->hot = false;
    node->dirty = true;
    node->dead = false;
    int32_t sidx = node->id % SLOTNUM;
    LeafSlot* slot = lslots_ + sidx;
    slot->warm->set(node->id, node, LeafCache::MLAST);
    cusage_ += node->size;
    return node;
  }
  /**
   * Remove a leaf node from the cache.
   * @param node the leaf node.
   * @param save whether to save dirty node.
   * @return true on success, or false on failure.
   */
  bool flush_leaf_node(LeafNode* node, bool save) {
    _assert_(node);
    bool err = false;
    if (save && !save_leaf_node(node)) err = true;
    typename RecordArray::const_iterator rit = node->recs.begin();
    typename RecordArray::const_iterator ritend = node->recs.end();
    while (rit != ritend) {
      Record* rec = *rit;
      xfree(rec);
      ++rit;
    }
    int32_t sidx = node->id % SLOTNUM;
    LeafSlot* slot = lslots_ + sidx;
    if (node->hot) {
      slot->hot->remove(node->id);
    } else {
      slot->warm->remove(node->id);
    }
    cusage_ -= node->size;
    delete node;
    return !err;
  }
  /**
   * Save a leaf node.
   * @param node the leaf node.
   * @return true on success, or false on failure.
   */
  bool save_leaf_node(LeafNode* node) {
    _assert_(node);
    ScopedRWLock lock(&node->lock, false);
    if (!node->dirty) return true;
    bool err = false;
    char hbuf[NUMBUFSIZ];
    size_t hsiz = write_key(hbuf, LNPREFIX, node->id);
    if (node->dead) {
      if (!db_.remove(hbuf, hsiz) && db_.error().code() != Error::NOREC) err = true;
    } else {
      char* rbuf = new char[node->size];
      char* wp = rbuf;
      wp += writevarnum(wp, node->prev);
      wp += writevarnum(wp, node->next);
      typename RecordArray::const_iterator rit = node->recs.begin();
      typename RecordArray::const_iterator ritend = node->recs.end();
      while (rit != ritend) {
        Record* rec = *rit;
        wp += writevarnum(wp, rec->ksiz);
        wp += writevarnum(wp, rec->vsiz);
        char* dbuf = (char*)rec + sizeof(*rec);
        std::memcpy(wp, dbuf, rec->ksiz);
        wp += rec->ksiz;
        std::memcpy(wp, dbuf + rec->ksiz, rec->vsiz);
        wp += rec->vsiz;
        ++rit;
      }
      if (!db_.set(hbuf, hsiz, rbuf, wp - rbuf)) err = true;
      delete[] rbuf;
    }
    node->dirty = false;
    return !err;
  }
  /**
   * Load a leaf node.
   * @param id the ID number of the leaf node.
   * @param prom whether to promote the warm cache.
   * @return the loaded leaf node.
   */
  LeafNode* load_leaf_node(int64_t id, bool prom) {
    _assert_(id > 0);
    int32_t sidx = id % SLOTNUM;
    LeafSlot* slot = lslots_ + sidx;
    ScopedMutex lock(&slot->lock);
    LeafNode** np = slot->hot->get(id, LeafCache::MLAST);
    if (np) return *np;
    if (prom) {
      if (slot->hot->count() * WARMRATIO > slot->warm->count() + WARMRATIO) {
        slot->hot->first_value()->hot = false;
        slot->hot->migrate(slot->hot->first_key(), slot->warm, LeafCache::MLAST);
      }
      np = slot->warm->migrate(id, slot->hot, LeafCache::MLAST);
      if (np) {
        (*np)->hot = true;
        return *np;
      }
    } else {
      LeafNode** np = slot->warm->get(id, LeafCache::MLAST);
      if (np) return *np;
    }
    char hbuf[NUMBUFSIZ];
    size_t hsiz = write_key(hbuf, LNPREFIX, id);
    class VisitorImpl : public DB::Visitor {
     public:
      explicit VisitorImpl() : node_(NULL) {}
      LeafNode* pop() {
        return node_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        uint64_t prev;
        size_t step = readvarnum(vbuf, vsiz, &prev);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        uint64_t next;
        step = readvarnum(vbuf, vsiz, &next);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        LeafNode* node = new LeafNode;
        node->size = sizeof(int32_t) * 2;
        node->prev = prev;
        node->next = next;
        while (vsiz > 1) {
          uint64_t rksiz;
          step = readvarnum(vbuf, vsiz, &rksiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          uint64_t rvsiz;
          step = readvarnum(vbuf, vsiz, &rvsiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          if (vsiz < rksiz + rvsiz) break;
          size_t rsiz = sizeof(Record) + rksiz + rvsiz;
          Record* rec = (Record*)xmalloc(rsiz);
          rec->ksiz = rksiz;
          rec->vsiz = rvsiz;
          char* dbuf = (char*)rec + sizeof(*rec);
          std::memcpy(dbuf, vbuf, rksiz);
          vbuf += rksiz;
          vsiz -= rksiz;
          std::memcpy(dbuf + rksiz, vbuf, rvsiz);
          vbuf += rvsiz;
          vsiz -= rvsiz;
          node->recs.push_back(rec);
          node->size += rsiz;
        }
        if (vsiz != 0) {
          typename RecordArray::const_iterator rit = node->recs.begin();
          typename RecordArray::const_iterator ritend = node->recs.end();
          while (rit != ritend) {
            Record* rec = *rit;
            xfree(rec);
            ++rit;
          }
          delete node;
          return NOP;
        }
        node_ = node;
        return NOP;
      }
      LeafNode* node_;
    } visitor;
    if (!db_.accept(hbuf, hsiz, &visitor, false)) return NULL;
    LeafNode* node = visitor.pop();
    if (!node) return NULL;
    node->id = id;
    node->hot = false;
    node->dirty = false;
    node->dead = false;
    slot->warm->set(id, node, LeafCache::MLAST);
    cusage_ += node->size;
    return node;
  }
  /**
   * Check whether a record is in the range of a leaf node.
   * @param node the leaf node.
   * @param rec the record containing the key only.
   * @return true for in range, or false for out of range.
   */
  bool check_leaf_node_range(LeafNode* node, Record* rec) {
    _assert_(node && rec);
    RecordArray& recs = node->recs;
    if (recs.empty()) return false;
    Record* frec = recs.front();
    Record* lrec = recs.back();
    return !reccomp_(rec, frec) && !reccomp_(lrec, rec);
  }
  /**
   * Accept a visitor at a leaf node.
   * @param node the leaf node.
   * @param rec the record containing the key only.
   * @param visitor a visitor object.
   * @return true to reorganize the tree, or false if not.
   */
  bool accept_impl(LeafNode* node, Record* rec, Visitor* visitor) {
    _assert_(node && rec && visitor);
    bool reorg = false;
    RecordArray& recs = node->recs;
    typename RecordArray::iterator ritend = recs.end();
    typename RecordArray::iterator rit = std::lower_bound(recs.begin(), ritend, rec, reccomp_);
    if (rit != ritend && !reccomp_(rec, *rit)) {
      Record* rec = *rit;
      char* kbuf = (char*)rec + sizeof(*rec);
      size_t ksiz = rec->ksiz;
      size_t vsiz;
      const char* vbuf = visitor->visit_full(kbuf, ksiz, kbuf + ksiz, rec->vsiz, &vsiz);
      if (vbuf == Visitor::REMOVE) {
        size_t rsiz = sizeof(*rec) + rec->ksiz + rec->vsiz;
        count_ -= 1;
        cusage_ -= rsiz;
        node->size -= rsiz;
        node->dirty = true;
        xfree(rec);
        recs.erase(rit);
        if (recs.empty()) reorg = true;
      } else if (vbuf != Visitor::NOP) {
        int64_t diff = (int64_t)vsiz - (int64_t)rec->vsiz;
        cusage_ += diff;
        node->size += diff;
        node->dirty = true;
        if (vsiz > rec->vsiz) {
          *rit = (Record*)xrealloc(rec, sizeof(*rec) + rec->ksiz + vsiz);
          rec = *rit;
          kbuf = (char*)rec + sizeof(*rec);
        }
        std::memcpy(kbuf + rec->ksiz, vbuf, vsiz);
        rec->vsiz = vsiz;
        if (node->size > psiz_ && recs.size() > 1) reorg = true;
      }
    } else {
      const char* kbuf = (char*)rec + sizeof(*rec);
      size_t ksiz = rec->ksiz;
      size_t vsiz;
      const char* vbuf = visitor->visit_empty(kbuf, ksiz, &vsiz);
      if (vbuf != Visitor::NOP && vbuf != Visitor::REMOVE) {
        size_t rsiz = sizeof(*rec) + ksiz + vsiz;
        count_ += 1;
        cusage_ += rsiz;
        node->size += rsiz;
        node->dirty = true;
        rec = (Record*)xmalloc(rsiz);
        rec->ksiz = ksiz;
        rec->vsiz = vsiz;
        char* dbuf = (char*)rec + sizeof(*rec);
        std::memcpy(dbuf, kbuf, ksiz);
        std::memcpy(dbuf + ksiz, vbuf, vsiz);
        recs.insert(rit, rec);
        if (node->size > psiz_ && recs.size() > 1) reorg = true;
      }
    }
    return reorg;
  }
  /**
   * Devide a leaf node into two.
   * @param node the leaf node.
   * @return the created node, or NULL on failure.
   */
  LeafNode* divide_leaf_node(LeafNode* node) {
    _assert_(node);
    LeafNode* newnode = create_leaf_node(node->id, node->next);
    if (newnode->next > 0) {
      LeafNode* nextnode = load_leaf_node(newnode->next, false);
      if (!nextnode) {
        set_error(_KCCODELINE_, Error::BROKEN, "missing leaf node");
        db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)newnode->next);
        return NULL;
      }
      nextnode->prev = newnode->id;
      nextnode->dirty = true;
    }
    node->next = newnode->id;
    node->dirty = true;
    RecordArray& recs = node->recs;
    typename RecordArray::iterator mid = recs.begin() + recs.size() / 2;
    typename RecordArray::iterator rit = mid;
    typename RecordArray::iterator ritend = recs.end();
    RecordArray& newrecs = newnode->recs;
    while (rit != ritend) {
      Record* rec = *rit;
      newrecs.push_back(rec);
      size_t rsiz = sizeof(*rec) + rec->ksiz + rec->vsiz;
      node->size -= rsiz;
      newnode->size += rsiz;
      ++rit;
    }
    escape_cursors(node->id, node->next, *mid);
    recs.erase(mid, ritend);
    return newnode;
  }
  /**
   * Open the inner cache.
   */
  void create_inner_cache() {
    _assert_(true);
    int64_t bnum = (bnum_ / AVGWAY) / SLOTNUM + 1;
    if (bnum < INT8MAX) bnum = INT8MAX;
    bnum = nearbyprime(bnum);
    for (int32_t i = 0; i < SLOTNUM; i++) {
      islots_[i].warm = new InnerCache(bnum);
    }
  }
  /**
   * Close the inner cache.
   */
  void delete_inner_cache() {
    _assert_(true);
    for (int32_t i = SLOTNUM - 1; i >= 0; i--) {
      InnerSlot* slot = islots_ + i;
      delete slot->warm;
    }
  }
  /**
   * Remove all inner nodes from the inner cache.
   * @param save whether to save dirty nodes.
   * @return true on success, or false on failure.
   */
  bool flush_inner_cache(bool save) {
    _assert_(true);
    bool err = false;
    for (int32_t i = SLOTNUM - 1; i >= 0; i--) {
      InnerSlot* slot = islots_ + i;
      typename InnerCache::Iterator it = slot->warm->begin();
      typename InnerCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        InnerNode* node = it.value();
        ++it;
        if (!flush_inner_node(node, save)) err = true;
      }
    }
    return !err;
  }
  /**
   * Flush a part of the inner cache.
   * @param slot a slot of inner nodes.
   * @return true on success, or false on failure.
   */
  bool flush_inner_cache_part(InnerSlot* slot) {
    _assert_(slot);
    bool err = false;
    if (slot->warm->count() > 0) {
      InnerNode* node = slot->warm->first_value();
      if (!flush_inner_node(node, true)) err = true;
    }
    return !err;
  }
  /**
   * Clean all of the inner cache.
   * @return true on success, or false on failure.
   */
  bool clean_inner_cache() {
    _assert_(true);
    bool err = false;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      InnerSlot* slot = islots_ + i;
      ScopedMutex lock(&slot->lock);
      typename InnerCache::Iterator it = slot->warm->begin();
      typename InnerCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        InnerNode* node = it.value();
        if (!save_inner_node(node)) err = true;
        ++it;
      }
    }
    return !err;
  }
  /**
   * Create a new inner node.
   * @param heir the ID of the child before the first link.
   * @return the created inner node.
   */
  InnerNode* create_inner_node(int64_t heir) {
    _assert_(true);
    InnerNode* node = new InnerNode;
    node->id = ++icnt_ + INIDBASE;
    node->heir = heir;
    node->links.reserve(DEFIINUM);
    node->size = sizeof(int64_t);
    node->dirty = true;
    node->dead = false;
    int32_t sidx = node->id % SLOTNUM;
    InnerSlot* slot = islots_ + sidx;
    slot->warm->set(node->id, node, InnerCache::MLAST);
    cusage_ += node->size;
    return node;
  }
  /**
   * Remove an inner node from the cache.
   * @param node the inner node.
   * @param save whether to save dirty node.
   * @return true on success, or false on failure.
   */
  bool flush_inner_node(InnerNode* node, bool save) {
    _assert_(node);
    bool err = false;
    if (save && !save_inner_node(node)) err = true;
    typename LinkArray::const_iterator lit = node->links.begin();
    typename LinkArray::const_iterator litend = node->links.end();
    while (lit != litend) {
      Link* link = *lit;
      xfree(link);
      ++lit;
    }
    int32_t sidx = node->id % SLOTNUM;
    InnerSlot* slot = islots_ + sidx;
    slot->warm->remove(node->id);
    cusage_ -= node->size;
    delete node;
    return !err;
  }
  /**
   * Save a inner node.
   * @param node the inner node.
   * @return true on success, or false on failure.
   */
  bool save_inner_node(InnerNode* node) {
    _assert_(true);
    if (!node->dirty) return true;
    bool err = false;
    char hbuf[NUMBUFSIZ];
    size_t hsiz = write_key(hbuf, INPREFIX, node->id - INIDBASE);
    if (node->dead) {
      if (!db_.remove(hbuf, hsiz) && db_.error().code() != Error::NOREC) err = true;
    } else {
      char* rbuf = new char[node->size];
      char* wp = rbuf;
      wp += writevarnum(wp, node->heir);
      typename LinkArray::const_iterator lit = node->links.begin();
      typename LinkArray::const_iterator litend = node->links.end();
      while (lit != litend) {
        Link* link = *lit;
        wp += writevarnum(wp, link->child);
        wp += writevarnum(wp, link->ksiz);
        char* dbuf = (char*)link + sizeof(*link);
        std::memcpy(wp, dbuf, link->ksiz);
        wp += link->ksiz;
        ++lit;
      }
      if (!db_.set(hbuf, hsiz, rbuf, wp - rbuf)) err = true;
      delete[] rbuf;
    }
    node->dirty = false;
    return !err;
  }
  /**
   * Load an inner node.
   * @param id the ID number of the inner node.
   * @return the loaded inner node.
   */
  InnerNode* load_inner_node(int64_t id) {
    _assert_(id > 0);
    int32_t sidx = id % SLOTNUM;
    InnerSlot* slot = islots_ + sidx;
    ScopedMutex lock(&slot->lock);
    InnerNode** np = slot->warm->get(id, InnerCache::MLAST);
    if (np) return *np;
    char hbuf[NUMBUFSIZ];
    size_t hsiz = write_key(hbuf, INPREFIX, id - INIDBASE);
    class VisitorImpl : public DB::Visitor {
     public:
      explicit VisitorImpl() : node_(NULL) {}
      InnerNode* pop() {
        return node_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        uint64_t heir;
        size_t step = readvarnum(vbuf, vsiz, &heir);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        InnerNode* node = new InnerNode;
        node->size = sizeof(int64_t);
        node->heir = heir;
        while (vsiz > 1) {
          uint64_t child;
          step = readvarnum(vbuf, vsiz, &child);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          uint64_t rksiz;
          step = readvarnum(vbuf, vsiz, &rksiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          if (vsiz < rksiz) break;
          Link* link = (Link*)xmalloc(sizeof(*link) + rksiz);
          link->child = child;
          link->ksiz = rksiz;
          char* dbuf = (char*)link + sizeof(*link);
          std::memcpy(dbuf, vbuf, rksiz);
          vbuf += rksiz;
          vsiz -= rksiz;
          node->links.push_back(link);
          node->size += sizeof(*link) + rksiz;
        }
        if (vsiz != 0) {
          typename LinkArray::const_iterator lit = node->links.begin();
          typename LinkArray::const_iterator litend = node->links.end();
          while (lit != litend) {
            Link* link = *lit;
            xfree(link);
            ++lit;
          }
          delete node;
          return NOP;
        }
        node_ = node;
        return NOP;
      }
      InnerNode* node_;
    } visitor;
    if (!db_.accept(hbuf, hsiz, &visitor, false)) return NULL;
    InnerNode* node = visitor.pop();
    if (!node) return NULL;
    node->id = id;
    node->dirty = false;
    node->dead = false;
    slot->warm->set(id, node, InnerCache::MLAST);
    cusage_ += node->size;
    return node;
  }
  /**
   * Search the B+ tree.
   * @param link the link containing the key only.
   * @param prom whether to promote the warm cache.
   * @param hist the array of visiting history.
   * @param hnp the pointer to the variable into which the number of the history is assigned.
   * @return the corresponding leaf node, or NULL on failure.
   */
  LeafNode* search_tree(Link* link, bool prom, int64_t* hist, int32_t* hnp) {
    _assert_(link && hist && hnp);
    int64_t id = root_;
    int32_t hnum = 0;
    while (id > INIDBASE) {
      InnerNode* node = load_inner_node(id);
      if (!node) {
        set_error(_KCCODELINE_, Error::BROKEN, "missing inner node");
        db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)id);
        return NULL;
      }
      hist[hnum++] = id;
      const LinkArray& links = node->links;
      typename LinkArray::const_iterator litbeg = links.begin();
      typename LinkArray::const_iterator litend = links.end();
      typename LinkArray::const_iterator lit = std::upper_bound(litbeg, litend, link, linkcomp_);
      if (lit == litbeg) {
        id = node->heir;
      } else {
        --lit;
        Link* link = *lit;
        id = link->child;
      }
    }
    *hnp = hnum;
    return load_leaf_node(id, prom);
  }
  /**
   * Reorganize the B+ tree.
   * @param node a leaf node.
   * @param hist the array of visiting history.
   * @param hnum the number of the history.
   * @return true on success, or false on failure.
   */
  bool reorganize_tree(LeafNode* node, int64_t* hist, int32_t hnum) {
    _assert_(node && hist && hnum >= 0);
    if (node->size > psiz_ && node->recs.size() > 1) {
      LeafNode* newnode = divide_leaf_node(node);
      if (!newnode) return false;
      if (node->id == last_) last_ = newnode->id;
      int64_t heir = node->id;
      int64_t child = newnode->id;
      Record* rec = *newnode->recs.begin();
      char* dbuf = (char*)rec + sizeof(*rec);
      int32_t ksiz = rec->ksiz;
      char* kbuf = new char[ksiz];
      std::memcpy(kbuf, dbuf, ksiz);
      while (true) {
        if (hnum < 1) {
          InnerNode* inode = create_inner_node(heir);
          add_link_inner_node(inode, child, kbuf, ksiz);
          root_ = inode->id;
          delete[] kbuf;
          break;
        }
        int64_t parent = hist[--hnum];
        InnerNode* inode = load_inner_node(parent);
        if (!inode) {
          set_error(_KCCODELINE_, Error::BROKEN, "missing inner node");
          db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)parent);
          delete[] kbuf;
          return false;
        }
        add_link_inner_node(inode, child, kbuf, ksiz);
        delete[] kbuf;
        LinkArray& links = inode->links;
        if (inode->size <= psiz_ || links.size() <= INLINKMIN) break;
        typename LinkArray::iterator litbeg = links.begin();
        typename LinkArray::iterator mid = litbeg + links.size() / 2;
        Link* link = *mid;
        InnerNode* newinode = create_inner_node(link->child);
        heir = inode->id;
        child = newinode->id;
        char* dbuf = (char*)link + sizeof(*link);
        ksiz = link->ksiz;
        kbuf = new char[ksiz];
        std::memcpy(kbuf, dbuf, ksiz);
        typename LinkArray::iterator lit = mid + 1;
        typename LinkArray::iterator litend = links.end();
        while (lit != litend) {
          link = *lit;
          char* dbuf = (char*)link + sizeof(*link);
          add_link_inner_node(newinode, link->child, dbuf, link->ksiz);
          ++lit;
        }
        int32_t num = newinode->links.size();
        for (int32_t i = 0; i <= num; i++) {
          Link* link = links.back();
          size_t rsiz = sizeof(*link) + link->ksiz;
          cusage_ -= rsiz;
          inode->size -= rsiz;
          xfree(link);
          links.pop_back();
        }
        inode->dirty = true;
      }
    } else if (node->recs.empty() && hnum > 0) {
      if (!escape_cursors(node->id, node->next)) return false;
      InnerNode* inode = load_inner_node(hist[--hnum]);
      if (!inode) {
        set_error(_KCCODELINE_, Error::BROKEN, "missing inner node");
        db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)hist[hnum]);
        return false;
      }
      if (sub_link_tree(inode, node->id, hist, hnum)) {
        if (node->prev > 0) {
          LeafNode* tnode = load_leaf_node(node->prev, false);
          if (!tnode) {
            set_error(_KCCODELINE_, Error::BROKEN, "missing node");
            db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)node->prev);
            return false;
          }
          tnode->next = node->next;
          tnode->dirty = true;
          if (last_ == node->id) last_ = node->prev;
        }
        if (node->next > 0) {
          LeafNode* tnode = load_leaf_node(node->next, false);
          if (!tnode) {
            set_error(_KCCODELINE_, Error::BROKEN, "missing node");
            db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)node->next);
            return false;
          }
          tnode->prev = node->prev;
          tnode->dirty = true;
          if (first_ == node->id) first_ = node->next;
        }
        node->dead = true;
      }
    }
    return true;
  }
  /**
   * Add a link to a inner node.
   * @param node the inner node.
   * @param child the ID number of the child.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   */
  void add_link_inner_node(InnerNode* node, int64_t child, const char* kbuf, size_t ksiz) {
    _assert_(node && kbuf);
    size_t rsiz = sizeof(Link) + ksiz;
    Link* link = (Link*)xmalloc(rsiz);
    link->child = child;
    link->ksiz = ksiz;
    char* dbuf = (char*)link + sizeof(*link);
    std::memcpy(dbuf, kbuf, ksiz);
    LinkArray& links = node->links;
    typename LinkArray::iterator litend = links.end();
    typename LinkArray::iterator lit = std::upper_bound(links.begin(), litend, link, linkcomp_);
    links.insert(lit, link);
    node->size += rsiz;
    node->dirty = true;
    cusage_ += rsiz;
  }
  /**
   * Subtract a link from the B+ tree.
   * @param node the inner node.
   * @param child the ID number of the child.
   * @param hist the array of visiting history.
   * @param hnum the number of the history.
   * @return true on success, or false on failure.
   */
  bool sub_link_tree(InnerNode* node, int64_t child, int64_t* hist, int32_t hnum) {
    _assert_(node && hist && hnum >= 0);
    node->dirty = true;
    LinkArray& links = node->links;
    typename LinkArray::iterator lit = links.begin();
    typename LinkArray::iterator litend = links.end();
    if (node->heir == child) {
      if (!links.empty()) {
        Link* link = *lit;
        node->heir = link->child;
        xfree(link);
        links.erase(lit);
        return true;
      } else if (hnum > 0) {
        InnerNode* pnode = load_inner_node(hist[--hnum]);
        if (!pnode) {
          set_error(_KCCODELINE_, Error::BROKEN, "missing inner node");
          db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)hist[hnum]);
          return false;
        }
        node->dead = true;
        return sub_link_tree(pnode, node->id, hist, hnum);
      }
      node->dead = true;
      root_ = child;
      while (child > INIDBASE) {
        node = load_inner_node(child);
        if (!node) {
          set_error(_KCCODELINE_, Error::BROKEN, "missing inner node");
          db_.report(_KCCODELINE_, Logger::WARN, "id=%lld", (long long)child);
          return false;
        }
        if (node->dead) {
          child = node->heir;
          root_ = child;
        } else {
          child = 0;
        }
      }
      return false;
    }
    while (lit != litend) {
      Link* link = *lit;
      if (link->child == child) {
        xfree(link);
        links.erase(lit);
        return true;
      }
      ++lit;
    }
    set_error(_KCCODELINE_, Error::BROKEN, "invalid tree");
    return false;
  }
  /**
   * Dump the meta data into the file.
   * @return true on success, or false on failure.
   */
  bool dump_meta() {
    _assert_(true);
    char head[HEADSIZ];
    std::memset(head, 0, sizeof(head));
    char* wp = head;
    if (reccomp_.comp == LEXICALCOMP) {
      *(uint8_t*)(wp++) = 0x10;
    } else if (reccomp_.comp == DECIMALCOMP) {
      *(uint8_t*)(wp++) = 0x11;
    } else if (reccomp_.comp == LEXICALDESCCOMP) {
      *(uint8_t*)(wp++) = 0x18;
    } else if (reccomp_.comp == DECIMALDESCCOMP) {
      *(uint8_t*)(wp++) = 0x19;
    } else {
      *(uint8_t*)(wp++) = 0xff;
    }
    wp = head + MOFFNUMS;
    uint64_t num = hton64(psiz_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(root_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(first_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(last_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(lcnt_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(icnt_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(count_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    num = hton64(bnum_);
    std::memcpy(wp, &num, sizeof(num));
    wp += sizeof(num);
    std::memcpy(wp, "\x0a\x42\x6f\x6f\x66\x79\x21\x0a", sizeof(num));
    wp += sizeof(num);
    if (!db_.set(KCPDBMETAKEY, sizeof(KCPDBMETAKEY) - 1, head, sizeof(head))) return false;
    trlcnt_ = lcnt_;
    trcount_ = count_;
    return true;
  }
  /**
   * Load the meta data from the file.
   * @return true on success, or false on failure.
   */
  bool load_meta() {
    _assert_(true);
    char head[HEADSIZ];
    int32_t hsiz = db_.get(KCPDBMETAKEY, sizeof(KCPDBMETAKEY) - 1, head, sizeof(head));
    if (hsiz < 0) return false;
    if (hsiz != sizeof(head)) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid meta data record");
      db_.report(_KCCODELINE_, Logger::WARN, "hsiz=%d", hsiz);
      return false;
    }
    const char* rp = head;
    if (*(uint8_t*)rp == 0x10) {
      reccomp_.comp = LEXICALCOMP;
      linkcomp_.comp = LEXICALCOMP;
    } else if (*(uint8_t*)rp == 0x11) {
      reccomp_.comp = DECIMALCOMP;
      linkcomp_.comp = DECIMALCOMP;
    } else if (*(uint8_t*)rp == 0x18) {
      reccomp_.comp = LEXICALDESCCOMP;
      linkcomp_.comp = LEXICALDESCCOMP;
    } else if (*(uint8_t*)rp == 0x19) {
      reccomp_.comp = DECIMALDESCCOMP;
      linkcomp_.comp = DECIMALDESCCOMP;
    } else if (*(uint8_t*)rp == 0xff) {
      if (!reccomp_.comp) {
        set_error(_KCCODELINE_, Error::INVALID, "the custom comparator is not given");
        return false;
      }
      linkcomp_.comp = reccomp_.comp;
    } else {
      set_error(_KCCODELINE_, Error::BROKEN, "comparator is invalid");
      return false;
    }
    rp = head + MOFFNUMS;
    uint64_t num;
    std::memcpy(&num, rp, sizeof(num));
    psiz_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    root_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    first_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    last_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    lcnt_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    icnt_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    count_ = ntoh64(num);
    rp += sizeof(num);
    std::memcpy(&num, rp, sizeof(num));
    bnum_ = ntoh64(num);
    rp += sizeof(num);
    trlcnt_ = lcnt_;
    trcount_ = count_;
    return true;
  }
  /**
   * Caluculate the total number of nodes in the leaf cache.
   * @return the total number of nodes in the leaf cache.
   */
  int64_t calc_leaf_cache_count() {
    _assert_(true);
    int64_t sum = 0;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      LeafSlot* slot = lslots_ + i;
      sum += slot->warm->count();
      sum += slot->hot->count();
    }
    return sum;
  }
  /**
   * Caluculate the amount of memory usage of the leaf cache.
   * @return the amount of memory usage of the leaf cache.
   */
  int64_t calc_leaf_cache_size() {
    _assert_(true);
    int64_t sum = 0;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      LeafSlot* slot = lslots_ + i;
      typename LeafCache::Iterator it = slot->warm->begin();
      typename LeafCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        LeafNode* node = it.value();
        sum += node->size;
        ++it;
      }
      it = slot->hot->begin();
      itend = slot->hot->end();
      while (it != itend) {
        LeafNode* node = it.value();
        sum += node->size;
        ++it;
      }
    }
    return sum;
  }
  /**
   * Caluculate the total number of nodes in the inner cache.
   * @return the total number of nodes in the inner cache.
   */
  int64_t calc_inner_cache_count() {
    _assert_(true);
    int64_t sum = 0;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      InnerSlot* slot = islots_ + i;
      sum += slot->warm->count();
    }
    return sum;
  }
  /**
   * Caluculate the amount of memory usage of the inner cache.
   * @return the amount of memory usage of the inner cache.
   */
  int64_t calc_inner_cache_size() {
    _assert_(true);
    int64_t sum = 0;
    for (int32_t i = 0; i < SLOTNUM; i++) {
      InnerSlot* slot = islots_ + i;
      typename InnerCache::Iterator it = slot->warm->begin();
      typename InnerCache::Iterator itend = slot->warm->end();
      while (it != itend) {
        InnerNode* node = it.value();
        sum += node->size;
        ++it;
      }
    }
    return sum;
  }
  /**
   * Disable all cursors.
   */
  void disable_cursors() {
    _assert_(true);
    if (curs_.empty()) return;
    typename CursorList::const_iterator cit = curs_.begin();
    typename CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      if (cur->kbuf_) cur->clear_position();
      ++cit;
    }
  }
  /**
   * Escape cursors on a divided leaf node.
   * @param src the ID of the source node.
   * @param dest the ID of the destination node.
   * @param rec the pivot record.
   * @return true on success, or false on failure.
   */
  void escape_cursors(int64_t src, int64_t dest, Record* rec) {
    _assert_(src > 0 && dest >= 0 && rec);
    if (curs_.empty()) return;
    typename CursorList::const_iterator cit = curs_.begin();
    typename CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      if (cur->lid_ == src) {
        char* dbuf = (char*)rec + sizeof(*rec);
        if (reccomp_.comp->compare(cur->kbuf_, cur->ksiz_, dbuf, rec->ksiz) >= 0)
          cur->lid_ = dest;
      }
      ++cit;
    }
  }
  /**
   * Escape cursors on a removed leaf node.
   * @param src the ID of the source node.
   * @param dest the ID of the destination node.
   * @return true on success, or false on failure.
   */
  bool escape_cursors(int64_t src, int64_t dest) {
    _assert_(src > 0 && dest >= 0);
    if (curs_.empty()) return true;
    bool err = false;
    typename CursorList::const_iterator cit = curs_.begin();
    typename CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      if (cur->lid_ == src) {
        cur->clear_position();
        if (!cur->set_position(dest) && db_.error().code() != Error::NOREC) err = true;
      }
      ++cit;
    }
    return !err;
  }
  /**
   * Recalculate the count data.
   * @return true on success, or false on failure.
   */
  bool recalc_count() {
    _assert_(true);
    if (!load_meta()) return false;
    bool err = false;
    std::set<int64_t> ids;
    std::set<int64_t> prevs;
    std::set<int64_t> nexts;
    class VisitorImpl : public DB::Visitor {
     public:
      explicit VisitorImpl(std::set<int64_t>* ids,
                           std::set<int64_t>* prevs, std::set<int64_t>* nexts) :
          ids_(ids), prevs_(prevs), nexts_(nexts), count_(0) {}
      int64_t count() {
        return count_;
      }
     private:
      const char* visit_full(const char* kbuf, size_t ksiz,
                             const char* vbuf, size_t vsiz, size_t* sp) {
        if (ksiz < 2 || ksiz >= NUMBUFSIZ || kbuf[0] != LNPREFIX) return NOP;
        kbuf++;
        ksiz--;
        char tkbuf[NUMBUFSIZ];
        std::memcpy(tkbuf, kbuf, ksiz);
        tkbuf[ksiz] = '\0';
        int64_t id = atoih(tkbuf);
        uint64_t prev;
        size_t step = readvarnum(vbuf, vsiz, &prev);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        uint64_t next;
        step = readvarnum(vbuf, vsiz, &next);
        if (step < 1) return NOP;
        vbuf += step;
        vsiz -= step;
        ids_->insert(id);
        if (prev > 0) prevs_->insert(prev);
        if (next > 0) nexts_->insert(next);
        while (vsiz > 1) {
          uint64_t rksiz;
          step = readvarnum(vbuf, vsiz, &rksiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          uint64_t rvsiz;
          step = readvarnum(vbuf, vsiz, &rvsiz);
          if (step < 1) break;
          vbuf += step;
          vsiz -= step;
          if (vsiz < rksiz + rvsiz) break;
          vbuf += rksiz;
          vsiz -= rksiz;
          vbuf += rvsiz;
          vsiz -= rvsiz;
          count_++;
        }
        return NOP;
      }
      std::set<int64_t>* ids_;
      std::set<int64_t>* prevs_;
      std::set<int64_t>* nexts_;
      int64_t count_;
    } visitor(&ids, &prevs, &nexts);
    if (!db_.iterate(&visitor, false)) err = true;
    int64_t count = visitor.count();
    db_.report(_KCCODELINE_, Logger::WARN, "recalculated the record count from %lld to %lld",
               (long long)count_, (long long)count);
    std::set<int64_t>::iterator iitend = ids.end();
    std::set<int64_t>::iterator nit = nexts.begin();
    std::set<int64_t>::iterator nitend = nexts.end();
    while (nit != nitend) {
      if (ids.find(*nit) == ids.end()) {
        db_.report(_KCCODELINE_, Logger::WARN, "detected missing leaf: %lld", (long long)*nit);
        count = INT64MAX;
      }
      ++nit;
    }
    std::set<int64_t>::iterator pit = prevs.begin();
    std::set<int64_t>::iterator pitend = prevs.end();
    while (pit != pitend) {
      if (ids.find(*pit) == iitend) {
        db_.report(_KCCODELINE_, Logger::WARN, "detected missing leaf: %lld", (long long)*pit);
        count = INT64MAX;
      }
      ++pit;
    }
    count_ = count;
    if (!dump_meta()) err = true;
    return !err;
  }
  /**
   * Reorganize the database file.
   * @param mode the connection mode of the internal database.
   * @return true on success, or false on failure.
   */
  bool reorganize_file(uint32_t mode) {
    _assert_(true);
    if (!load_meta()) {
      if (reccomp_.comp) {
        linkcomp_.comp = reccomp_.comp;
      } else {
        reccomp_.comp = LEXICALCOMP;
        linkcomp_.comp = LEXICALCOMP;
      }
    }
    const std::string& path = db_.path();
    const std::string& npath = path + File::EXTCHR + KCPDBTMPPATHEXT;
    PlantDB tdb;
    tdb.tune_comparator(reccomp_.comp);
    if (!tdb.open(npath, OWRITER | OCREATE | OTRUNCATE)) {
      set_error(_KCCODELINE_, tdb.error().code(), "opening the destination failed");
      return false;
    }
    db_.report(_KCCODELINE_, Logger::WARN, "reorganizing the database");
    bool err = false;
    create_leaf_cache();
    create_inner_cache();
    DB::Cursor* cur = db_.cursor();
    cur->jump();
    char* kbuf;
    size_t ksiz;
    while (!err && (kbuf = cur->get_key(&ksiz)) != NULL) {
      if (*kbuf == LNPREFIX) {
        int64_t id = std::strtol(kbuf + 1, NULL, 16);
        if (id > 0 && id < INIDBASE) {
          LeafNode* node = load_leaf_node(id, false);
          if (node) {
            const RecordArray& recs = node->recs;
            typename RecordArray::const_iterator rit = recs.begin();
            typename RecordArray::const_iterator ritend = recs.end();
            while (rit != ritend) {
              Record* rec = *rit;
              char* dbuf = (char*)rec + sizeof(*rec);
              if (!tdb.set(dbuf, rec->ksiz, dbuf + rec->ksiz, rec->vsiz)) {
                set_error(_KCCODELINE_, tdb.error().code(),
                          "opening the destination failed");
                err = true;
              }
              ++rit;
            }
            flush_leaf_node(node, false);
          }
        }
      }
      delete[] kbuf;
      cur->step();
    }
    delete cur;
    delete_inner_cache();
    delete_leaf_cache();
    if (!tdb.close()) {
      set_error(_KCCODELINE_, tdb.error().code(), "opening the destination failed");
      err = true;
    }
    if (DBTYPE == TYPETREE) {
      if (File::rename(npath, path)) {
        if (!db_.close()) err = true;
        if (!db_.open(path, mode)) err = true;
      } else {
        set_error(_KCCODELINE_, Error::SYSTEM, "renaming the destination failed");
        err = true;
      }
      File::remove(npath);
    } else if (DBTYPE == TYPEFOREST) {
      const std::string& tpath = npath + File::EXTCHR + KCPDBTMPPATHEXT;
      File::remove_recursively(tpath);
      if (File::rename(path, tpath)) {
        if (File::rename(npath, path)) {
          if (!db_.close()) err = true;
          if (!db_.open(path, mode)) err = true;
        } else {
          set_error(_KCCODELINE_, Error::SYSTEM, "renaming the destination failed");
          File::rename(tpath, path);
          err = true;
        }
      } else {
        set_error(_KCCODELINE_, Error::SYSTEM, "renaming the source failed");
        err = true;
      }
      File::remove_recursively(tpath);
      File::remove_recursively(npath);
    } else {
      BASEDB udb;
      if (!err && udb.open(npath, OREADER)) {
        if (writer_) {
          if (!db_.clear()) err = true;
        } else {
          if (!db_.close()) err = true;
          uint32_t tmode = (mode & ~OREADER) | OWRITER | OCREATE | OTRUNCATE;
          if (!db_.open(path, tmode)) err = true;
        }
        cur = udb.cursor();
        cur->jump();
        const char* vbuf;
        size_t vsiz;
        while (!err && (kbuf = cur->get(&ksiz, &vbuf, &vsiz)) != NULL) {
          if (!db_.set(kbuf, ksiz, vbuf, vsiz)) err = true;
          delete[] kbuf;
          cur->step();
        }
        delete cur;
        if (writer_) {
          if (!db_.synchronize(false, NULL)) err = true;
        } else {
          if (!db_.close()) err = true;
          if (!db_.open(path, mode)) err = true;
        }
        if (!udb.close()) {
          set_error(_KCCODELINE_, udb.error().code(), "closing the destination failed");
          err = true;
        }
      } else {
        set_error(_KCCODELINE_, udb.error().code(), "opening the destination failed");
        err = true;
      }
      File::remove_recursively(npath);
    }
    return !err;
  }
  /**
   * Begin transaction.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @return true on success, or false on failure.
   */
  bool begin_transaction_impl(bool hard) {
    _assert_(true);
    if (!clean_leaf_cache()) return false;
    if (!clean_inner_cache()) return false;
    int32_t idx = trclock_++ % SLOTNUM;
    LeafSlot* lslot = lslots_ + idx;
    if (lslot->warm->count() + lslot->hot->count() > 1) flush_leaf_cache_part(lslot);
    InnerSlot* islot = islots_ + idx;
    if (islot->warm->count() > 1) flush_inner_cache_part(islot);
    if ((trlcnt_ != lcnt_ || count_ != trcount_) && !dump_meta()) return false;
    if (!db_.begin_transaction(hard)) return false;
    return true;
  }
  /**
   * Commit transaction.
   * @return true on success, or false on failure.
   */
  bool commit_transaction() {
    _assert_(true);
    bool err = false;
    if (!clean_leaf_cache()) return false;
    if (!clean_inner_cache()) return false;
    if ((trlcnt_ != lcnt_ || count_ != trcount_) && !dump_meta()) err = true;
    if (!db_.end_transaction(true)) return false;
    return !err;
  }
  /**
   * Abort transaction.
   * @return true on success, or false on failure.
   */
  bool abort_transaction() {
    _assert_(true);
    bool err = false;
    flush_leaf_cache(false);
    flush_inner_cache(false);
    if (!db_.end_transaction(false)) err = true;
    if (!load_meta()) err = true;
    disable_cursors();
    return !err;
  }
  /**
   * Fix auto transaction for the B+ tree.
   * @return true on success, or false on failure.
   */
  bool fix_auto_transaction_tree() {
    _assert_(true);
    if (!db_.begin_transaction(autosync_)) return false;
    bool err = false;
    if (!clean_leaf_cache()) err = true;
    if (!clean_inner_cache()) err = true;
    size_t cnum = ATRANCNUM / SLOTNUM;
    int32_t idx = trclock_++ % SLOTNUM;
    LeafSlot* lslot = lslots_ + idx;
    if (lslot->warm->count() + lslot->hot->count() > cnum) flush_leaf_cache_part(lslot);
    InnerSlot* islot = islots_ + idx;
    if (islot->warm->count() > cnum) flush_inner_cache_part(islot);
    if (!dump_meta()) err = true;
    if (!db_.end_transaction(true)) err = true;
    return !err;
  }
  /**
   * Fix auto transaction for a leaf.
   * @return true on success, or false on failure.
   */
  bool fix_auto_transaction_leaf(LeafNode* node) {
    _assert_(node);
    bool err = false;
    if (!save_leaf_node(node)) err = true;
    return !err;
  }
  /**
   * Fix auto synchronization.
   * @return true on success, or false on failure.
   */
  bool fix_auto_synchronization() {
    _assert_(true);
    bool err = false;
    if (!flush_leaf_cache(true)) err = true;
    if (!flush_inner_cache(true)) err = true;
    if (!dump_meta()) err = true;
    if (!db_.synchronize(true, NULL)) err = true;
    return !err;
  }
  /**
   * Write the key pattern into a buffer.
   * @param kbuf the destination buffer.
   * @param pc the prefix character.
   * @param id the ID number of the page.
   * @return the size of the key pattern.
   */
  size_t write_key(char* kbuf, int32_t pc, int64_t num) {
    _assert_(kbuf && num >= 0);
    char* wp = kbuf;
    *(wp++) = pc;
    bool hit = false;
    for (size_t i = 0; i < sizeof(num); i++) {
      uint8_t c = num >> ((sizeof(num) - 1 - i) * 8);
      uint8_t h = c >> 4;
      if (h < 10) {
        if (hit || h != 0) {
          *(wp++) = '0' + h;
          hit = true;
        }
      } else {
        *(wp++) = 'A' - 10 + h;
        hit = true;
      }
      uint8_t l = c & 0xf;
      if (l < 10) {
        if (hit || l != 0) {
          *(wp++) = '0' + l;
          hit = true;
        }
      } else {
        *(wp++) = 'A' - 10 + l;
        hit = true;
      }
    }
    return wp - kbuf;
  }
  /** Dummy constructor to forbid the use. */
  PlantDB(const PlantDB&);
  /** Dummy Operator to forbid the use. */
  PlantDB& operator =(const PlantDB&);
  /** The method lock. */
  RWLock mlock_;
  /** The internal meta operation trigger. */
  MetaTrigger* mtrigger_;
  /** The open mode. */
  uint32_t omode_;
  /** The flag for writer. */
  bool writer_;
  /** The flag for auto transaction. */
  bool autotran_;
  /** The flag for auto synchronization. */
  bool autosync_;
  /** The internal database. */
  BASEDB db_;
  /** The cursor objects. */
  CursorList curs_;
  /** The alignment power. */
  uint8_t apow_;
  /** The free block pool power. */
  uint8_t fpow_;
  /** The options. */
  uint8_t opts_;
  /** The bucket number. */
  int64_t bnum_;
  /** The page size. */
  int32_t psiz_;
  /** The capacity of page cache. */
  int64_t pccap_;
  /** The root node. */
  int64_t root_;
  /** The first node. */
  int64_t first_;
  /** The last node. */
  int64_t last_;
  /** The count of leaf nodes. */
  int64_t lcnt_;
  /** The count of inner nodes. */
  int64_t icnt_;
  /** The record number. */
  AtomicInt64 count_;
  /** The cache memory usage. */
  AtomicInt64 cusage_;
  /** The Slots of leaf nodes. */
  LeafSlot lslots_[SLOTNUM];
  /** The Slots of inner nodes. */
  InnerSlot islots_[SLOTNUM];
  /** The record comparator. */
  RecordComparator reccomp_;
  /** The link comparator. */
  LinkComparator linkcomp_;
  /** The flag whether in transaction. */
  bool tran_;
  /** The logical clock for transaction. */
  int64_t trclock_;
  /** The leaf count history for transaction. */
  int64_t trlcnt_;
  /** The record count history for transaction. */
  int64_t trcount_;
};


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
