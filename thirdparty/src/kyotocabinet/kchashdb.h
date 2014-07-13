/*************************************************************************************************
 * File hash database
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


#ifndef _KCHASHDB_H                      // duplication check
#define _KCHASHDB_H

#include <kccommon.h>
#include <kcutil.h>
#include <kcthread.h>
#include <kcfile.h>
#include <kccompress.h>
#include <kccompare.h>
#include <kcmap.h>
#include <kcregex.h>
#include <kcdb.h>
#include <kcplantdb.h>

#define KCHDBMAGICDATA  "KC\n"           ///< magic data of the file
#define KCHDBCHKSUMSEED  "__kyotocabinet__"  ///< seed of the module checksum
#define KCHDBTMPPATHEXT  "tmpkch"        ///< extension of the temporary file

namespace kyotocabinet {                 // common namespace


/**
 * File hash database.
 * @note This class is a concrete class to operate a hash database on a file.  This class can be
 * inherited but overwriting methods is forbidden.  Before every database operation, it is
 * necessary to call the HashDB::open method in order to open a database file and connect the
 * database object to it.  To avoid data missing or corruption, it is important to close every
 * database file by the HashDB::close method when the database is no longer in use.  It is
 * forbidden for multible database objects in a process to open the same database at the same
 * time.  It is forbidden to share a database object with child processes.
 */
class HashDB : public BasicDB {
  friend class PlantDB<HashDB, BasicDB::TYPETREE>;
 public:
  class Cursor;
 private:
  struct Record;
  struct FreeBlock;
  struct FreeBlockComparator;
  class Repeater;
  class ScopedVisitor;
  /** An alias of set of free blocks. */
  typedef std::set<FreeBlock> FBP;
  /** An alias of list of cursors. */
  typedef std::list<Cursor*> CursorList;
  /** The offset of the library version. */
  static const int64_t MOFFLIBVER = 4;
  /** The offset of the library revision. */
  static const int64_t MOFFLIBREV = 5;
  /** The offset of the format revision. */
  static const int64_t MOFFFMTVER = 6;
  /** The offset of the module checksum. */
  static const int64_t MOFFCHKSUM = 7;
  /** The offset of the database type. */
  static const int64_t MOFFTYPE = 8;
  /** The offset of the alignment power. */
  static const int64_t MOFFAPOW = 9;
  /** The offset of the free block pool power. */
  static const int64_t MOFFFPOW = 10;
  /** The offset of the options. */
  static const int64_t MOFFOPTS = 11;
  /** The offset of the bucket number. */
  static const int64_t MOFFBNUM = 16;
  /** The offset of the status flags. */
  static const int64_t MOFFFLAGS = 24;
  /** The offset of the record number. */
  static const int64_t MOFFCOUNT = 32;
  /** The offset of the file size. */
  static const int64_t MOFFSIZE = 40;
  /** The offset of the opaque data. */
  static const int64_t MOFFOPAQUE = 48;
  /** The size of the header. */
  static const int64_t HEADSIZ = 64;
  /** The width of the free block. */
  static const int32_t FBPWIDTH = 6;
  /** The large width of the record address. */
  static const int32_t WIDTHLARGE = 6;
  /** The small width of the record address. */
  static const int32_t WIDTHSMALL = 4;
  /** The size of the record buffer. */
  static const size_t RECBUFSIZ = 48;
  /** The size of the IO buffer. */
  static const size_t IOBUFSIZ = 1024;
  /** The number of slots of the record lock. */
  static const int32_t RLOCKSLOT = 1024;
  /** The default alignment power. */
  static const uint8_t DEFAPOW = 3;
  /** The maximum alignment power. */
  static const uint8_t MAXAPOW = 15;
  /** The default free block pool power. */
  static const uint8_t DEFFPOW = 10;
  /** The maximum free block pool power. */
  static const uint8_t MAXFPOW = 20;
  /** The default bucket number. */
  static const int64_t DEFBNUM = 1048583LL;
  /** The default size of the memory-mapped region. */
  static const int64_t DEFMSIZ = 64LL << 20;
  /** The magic data for record. */
  static const uint8_t RECMAGIC = 0xcc;
  /** The magic data for padding. */
  static const uint8_t PADMAGIC = 0xee;
  /** The magic data for free block. */
  static const uint8_t FBMAGIC = 0xdd;
  /** The maximum unit of auto defragmentation. */
  static const int32_t DFRGMAX = 512;
  /** The coefficient of auto defragmentation. */
  static const int32_t DFRGCEF = 2;
  /** The checking width for record salvage. */
  static const int64_t SLVGWIDTH = 1LL << 20;
  /** The threshold of busy loop and sleep for locking. */
  static const uint32_t LOCKBUSYLOOP = 8192;
 public:
  /**
   * Cursor to indicate a record.
   */
  class Cursor : public BasicDB::Cursor {
    friend class HashDB;
   public:
    /**
     * Constructor.
     * @param db the container database object.
     */
    explicit Cursor(HashDB* db) : db_(db), off_(0), end_(0) {
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
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (writable) {
        if (!db_->writer_) {
          db_->set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
          return false;
        }
        if (!(db_->flags_ & FOPEN) && !db_->autotran_ && !db_->tran_ &&
            !db_->set_flag(FOPEN, true)) {
          return false;
        }
      }
      if (off_ < 1) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      Record rec;
      char rbuf[RECBUFSIZ];
      if (!step_impl(&rec, rbuf, 0)) return false;
      if (!rec.vbuf && !db_->read_record_body(&rec)) {
        delete[] rec.bbuf;
        return false;
      }
      const char* vbuf = rec.vbuf;
      size_t vsiz = rec.vsiz;
      char* zbuf = NULL;
      size_t zsiz = 0;
      if (db_->comp_) {
        zbuf = db_->comp_->decompress(vbuf, vsiz, &zsiz);
        if (!zbuf) {
          db_->set_error(_KCCODELINE_, Error::SYSTEM, "data decompression failed");
          delete[] rec.bbuf;
          return false;
        }
        vbuf = zbuf;
        vsiz = zsiz;
      }
      vbuf = visitor->visit_full(rec.kbuf, rec.ksiz, vbuf, vsiz, &vsiz);
      delete[] zbuf;
      if (vbuf == Visitor::REMOVE) {
        uint64_t hash = db_->hash_record(rec.kbuf, rec.ksiz);
        uint32_t pivot = db_->fold_hash(hash);
        int64_t bidx = hash % db_->bnum_;
        Repeater repeater(Visitor::REMOVE, 0);
        if (!db_->accept_impl(rec.kbuf, rec.ksiz, &repeater, bidx, pivot, true)) {
          delete[] rec.bbuf;
          return false;
        }
        delete[] rec.bbuf;
      } else if (vbuf == Visitor::NOP) {
        delete[] rec.bbuf;
        if (step) {
          if (step_impl(&rec, rbuf, 1)) {
            delete[] rec.bbuf;
          } else if (db_->error().code() != Error::NOREC) {
            return false;
          }
        }
      } else {
        zbuf = NULL;
        zsiz = 0;
        if (db_->comp_) {
          zbuf = db_->comp_->compress(vbuf, vsiz, &zsiz);
          if (!zbuf) {
            db_->set_error(_KCCODELINE_, Error::SYSTEM, "data compression failed");
            delete[] rec.bbuf;
            return false;
          }
          vbuf = zbuf;
          vsiz = zsiz;
        }
        size_t rsiz = db_->calc_record_size(rec.ksiz, vsiz);
        if (rsiz <= rec.rsiz) {
          rec.psiz = rec.rsiz - rsiz;
          rec.vsiz = vsiz;
          rec.vbuf = vbuf;
          if (!db_->adjust_record(&rec) || !db_->write_record(&rec, true)) {
            delete[] zbuf;
            delete[] rec.bbuf;
            return false;
          }
          delete[] zbuf;
          delete[] rec.bbuf;
          if (step) {
            if (step_impl(&rec, rbuf, 1)) {
              delete[] rec.bbuf;
            } else if (db_->error().code() != Error::NOREC) {
              return false;
            }
          }
        } else {
          uint64_t hash = db_->hash_record(rec.kbuf, rec.ksiz);
          uint32_t pivot = db_->fold_hash(hash);
          int64_t bidx = hash % db_->bnum_;
          Repeater repeater(vbuf, vsiz);
          if (!db_->accept_impl(rec.kbuf, rec.ksiz, &repeater, bidx, pivot, true)) {
            delete[] zbuf;
            delete[] rec.bbuf;
            return false;
          }
          delete[] zbuf;
          delete[] rec.bbuf;
        }
      }
      if (db_->dfunit_ > 0 && db_->frgcnt_ >= db_->dfunit_) {
        if (!db_->defrag_impl(db_->dfunit_ * DFRGCEF)) return false;
        db_->frgcnt_ -= db_->dfunit_;
      }
      return true;
    }
    /**
     * Jump the cursor to the first record for forward scan.
     * @return true on success, or false on failure.
     */
    bool jump() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      off_ = 0;
      if (db_->lsiz_ <= db_->roff_) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      off_ = db_->roff_;
      end_ = db_->lsiz_;
      return true;
    }
    /**
     * Jump the cursor to a record for forward scan.
     * @param kbuf the pointer to the key region.
     * @param ksiz the size of the key region.
     * @return true on success, or false on failure.
     */
    bool jump(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      off_ = 0;
      uint64_t hash = db_->hash_record(kbuf, ksiz);
      uint32_t pivot = db_->fold_hash(hash);
      int64_t bidx = hash % db_->bnum_;
      int64_t off = db_->get_bucket(bidx);
      if (off < 0) return false;
      Record rec;
      char rbuf[RECBUFSIZ];
      while (off > 0) {
        rec.off = off;
        if (!db_->read_record(&rec, rbuf)) return false;
        if (rec.psiz == UINT16MAX) {
          db_->set_error(_KCCODELINE_, Error::BROKEN, "free block in the chain");
          db_->report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
                      (long long)db_->psiz_, (long long)rec.off, (long long)db_->file_.size());
          return false;
        }
        uint32_t tpivot = db_->linear_ ? pivot :
            db_->fold_hash(db_->hash_record(rec.kbuf, rec.ksiz));
        if (pivot > tpivot) {
          delete[] rec.bbuf;
          off = rec.left;
        } else if (pivot < tpivot) {
          delete[] rec.bbuf;
          off = rec.right;
        } else {
          int32_t kcmp = db_->compare_keys(kbuf, ksiz, rec.kbuf, rec.ksiz);
          if (db_->linear_ && kcmp != 0) kcmp = 1;
          if (kcmp > 0) {
            delete[] rec.bbuf;
            off = rec.left;
          } else if (kcmp < 0) {
            delete[] rec.bbuf;
            off = rec.right;
          } else {
            delete[] rec.bbuf;
            off_ = off;
            end_ = db_->lsiz_;
            return true;
          }
        }
      }
      db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
      return false;
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
     * @note This is a dummy implementation for compatibility.
     */
    bool jump_back() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
      return false;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @note This is a dummy implementation for compatibility.
     */
    bool jump_back(const char* kbuf, size_t ksiz) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
      return false;
    }
    /**
     * Jump the cursor to a record for backward scan.
     * @note This is a dummy implementation for compatibility.
     */
    bool jump_back(const std::string& key) {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
      return false;
    }
    /**
     * Step the cursor to the next record.
     * @return true on success, or false on failure.
     */
    bool step() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      if (off_ < 1) {
        db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
        return false;
      }
      bool err = false;
      Record rec;
      char rbuf[RECBUFSIZ];
      if (step_impl(&rec, rbuf, 1)) {
        delete[] rec.bbuf;
      } else {
        err = true;
      }
      return !err;
    }
    /**
     * Step the cursor to the previous record.
     * @note This is a dummy implementation for compatibility.
     */
    bool step_back() {
      _assert_(true);
      ScopedRWLock lock(&db_->mlock_, true);
      if (db_->omode_ == 0) {
        db_->set_error(_KCCODELINE_, Error::INVALID, "not opened");
        return false;
      }
      db_->set_error(_KCCODELINE_, Error::NOIMPL, "not implemented");
      return false;
    }
    /**
     * Get the database object.
     * @return the database object.
     */
    HashDB* db() {
      _assert_(true);
      return db_;
    }
   private:
    /**
     * Step the cursor to the next record.
     * @param rec the record structure.
     * @param rbuf the working buffer.
     * @param skip the number of skipping blocks.
     * @return true on success, or false on failure.
     */
    bool step_impl(Record* rec, char* rbuf, int64_t skip) {
      _assert_(rec && rbuf && skip >= 0);
      if (off_ >= end_) {
        db_->set_error(_KCCODELINE_, Error::BROKEN, "cursor after the end");
        db_->report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
                    (long long)db_->psiz_, (long long)rec->off, (long long)db_->file_.size());
        return false;
      }
      while (off_ < end_) {
        rec->off = off_;
        if (!db_->read_record(rec, rbuf)) return false;
        skip--;
        if (rec->psiz == UINT16MAX) {
          off_ += rec->rsiz;
        } else {
          if (skip < 0) return true;
          delete[] rec->bbuf;
          off_ += rec->rsiz;
        }
      }
      db_->set_error(_KCCODELINE_, Error::NOREC, "no record");
      off_ = 0;
      return false;
    }
    /** Dummy constructor to forbid the use. */
    Cursor(const Cursor&);
    /** Dummy Operator to forbid the use. */
    Cursor& operator =(const Cursor&);
    /** The inner database. */
    HashDB* db_;
    /** The current offset. */
    int64_t off_;
    /** The end offset. */
    int64_t end_;
  };
  /**
   * Tuning options.
   */
  enum Option {
    TSMALL = 1 << 0,                     ///< use 32-bit addressing
    TLINEAR = 1 << 1,                    ///< use linear collision chaining
    TCOMPRESS = 1 << 2                   ///< compress each record
  };
  /**
   * Status flags.
   */
  enum Flag {
    FOPEN = 1 << 0,                      ///< whether opened
    FFATAL = 1 << 1                      ///< whether with fatal error
  };
  /**
   * Default constructor.
   */
  explicit HashDB() :
      mlock_(), rlock_(RLOCKSLOT), flock_(), atlock_(), error_(),
      logger_(NULL), logkinds_(0), mtrigger_(NULL),
      omode_(0), writer_(false), autotran_(false), autosync_(false),
      reorg_(false), trim_(false),
      file_(), fbp_(), curs_(), path_(""),
      libver_(0), librev_(0), fmtver_(0), chksum_(0), type_(TYPEHASH),
      apow_(DEFAPOW), fpow_(DEFFPOW), opts_(0), bnum_(DEFBNUM),
      flags_(0), flagopen_(false), count_(0), lsiz_(0), psiz_(0), opaque_(),
      msiz_(DEFMSIZ), dfunit_(0), embcomp_(ZLIBRAWCOMP),
      align_(0), fbpnum_(0), width_(0), linear_(false),
      comp_(NULL), rhsiz_(0), boff_(0), roff_(0), dfcur_(0), frgcnt_(0),
      tran_(false), trhard_(false), trfbp_(), trcount_(0), trsize_(0) {
    _assert_(true);
  }
  /**
   * Destructor.
   * @note If the database is not closed, it is closed implicitly.
   */
  virtual ~HashDB() {
    _assert_(true);
    if (omode_ != 0) close();
    if (!curs_.empty()) {
      CursorList::const_iterator cit = curs_.begin();
      CursorList::const_iterator citend = curs_.end();
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
    mlock_.lock_reader();
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    if (writable) {
      if (!writer_) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        mlock_.unlock();
        return false;
      }
      if (!(flags_ & FOPEN) && !autotran_ && !tran_ && !set_flag(FOPEN, true)) {
        mlock_.unlock();
        return false;
      }
    }
    bool err = false;
    uint64_t hash = hash_record(kbuf, ksiz);
    uint32_t pivot = fold_hash(hash);
    int64_t bidx = hash % bnum_;
    size_t lidx = bidx % RLOCKSLOT;
    if (writable) {
      rlock_.lock_writer(lidx);
    } else {
      rlock_.lock_reader(lidx);
    }
    if (!accept_impl(kbuf, ksiz, visitor, bidx, pivot, false)) err = true;
    rlock_.unlock(lidx);
    mlock_.unlock();
    if (!err && dfunit_ > 0 && frgcnt_ >= dfunit_ && mlock_.lock_writer_try()) {
      int64_t unit = frgcnt_;
      if (unit >= dfunit_) {
        if (unit > DFRGMAX) unit = DFRGMAX;
        if (!defrag_impl(unit * DFRGCEF)) err = true;
        frgcnt_ -= unit;
      }
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
    mlock_.lock_reader();
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      mlock_.unlock();
      return false;
    }
    if (writable) {
      if (!writer_) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        mlock_.unlock();
        return false;
      }
      if (!(flags_ & FOPEN) && !autotran_ && !tran_ && !set_flag(FOPEN, true)) {
        mlock_.unlock();
        return false;
      }
    }
    visitor->visit_before();
    size_t knum = keys.size();
    if (knum < 1) {
      visitor->visit_after();
      mlock_.unlock();
      return true;
    }
    bool err = false;
    struct RecordKey {
      const char* kbuf;
      size_t ksiz;
      uint32_t pivot;
      uint64_t bidx;
    };
    RecordKey* rkeys = new RecordKey[knum];
    std::set<size_t> lidxs;
    for (size_t i = 0; i < knum; i++) {
      const std::string& key = keys[i];
      RecordKey* rkey = rkeys + i;
      rkey->kbuf = key.data();
      rkey->ksiz = key.size();
      uint64_t hash = hash_record(rkey->kbuf, rkey->ksiz);
      rkey->pivot = fold_hash(hash);
      rkey->bidx = hash % bnum_;
      lidxs.insert(rkey->bidx % RLOCKSLOT);
    }
    std::set<size_t>::iterator lit = lidxs.begin();
    std::set<size_t>::iterator litend = lidxs.end();
    while (lit != litend) {
      if (writable) {
        rlock_.lock_writer(*lit);
      } else {
        rlock_.lock_reader(*lit);
      }
      ++lit;
    }
    for (size_t i = 0; i < knum; i++) {
      RecordKey* rkey = rkeys + i;
      if (!accept_impl(rkey->kbuf, rkey->ksiz, visitor, rkey->bidx, rkey->pivot, false)) {
        err = true;
        break;
      }
    }
    lit = lidxs.begin();
    litend = lidxs.end();
    while (lit != litend) {
      rlock_.unlock(*lit);
      ++lit;
    }
    delete[] rkeys;
    visitor->visit_after();
    mlock_.unlock();
    if (!err && dfunit_ > 0 && frgcnt_ >= dfunit_ && mlock_.lock_writer_try()) {
      int64_t unit = frgcnt_;
      if (unit >= dfunit_) {
        if (unit > DFRGMAX) unit = DFRGMAX;
        if (!defrag_impl(unit * DFRGCEF)) err = true;
        frgcnt_ -= unit;
      }
      mlock_.unlock();
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
    if (writable) {
      if (!writer_) {
        set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
        return false;
      }
      if (!(flags_ & FOPEN) && !autotran_ && !tran_ && !set_flag(FOPEN, true)) {
        mlock_.unlock();
        return false;
      }
    }
    ScopedVisitor svis(visitor);
    bool err = false;
    if (!iterate_impl(visitor, checker)) err = true;
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
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    if (thnum < 1) thnum = 1;
    if (thnum > (size_t)INT8MAX) thnum = INT8MAX;
    if ((int64_t)thnum > bnum_) thnum = bnum_;
    ScopedVisitor svis(visitor);
    rlock_.lock_reader_all();
    bool err = false;
    if (!scan_parallel_impl(visitor, thnum, checker)) err = true;
    rlock_.unlock_all();
    trigger_meta(MetaTrigger::ITERATE, "scan_parallel");
    return !err;
  }
  /**
   * Get the last happened error.
   * @return the last happened error.
   */
  Error error() const {
    _assert_(true);
    return error_;
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
    error_->set(code, message);
    if (code == Error::BROKEN || code == Error::SYSTEM) flags_ |= FFATAL;
    if (logger_) {
      Logger::Kind kind = code == Error::BROKEN || code == Error::SYSTEM ?
          Logger::ERROR : Logger::INFO;
      if (kind & logkinds_)
        report(file, line, func, kind, "%d: %s: %s", code, Error::codename(code), message);
    }
  }
  /**
   * Open a database file.
   * @param path the path of a database file.
   * @param mode the connection mode.  HashDB::OWRITER as a writer, HashDB::OREADER as a
   * reader.  The following may be added to the writer mode by bitwise-or: HashDB::OCREATE,
   * which means it creates a new database if the file does not exist, HashDB::OTRUNCATE, which
   * means it creates a new database regardless if the file exists, HashDB::OAUTOTRAN, which
   * means each updating operation is performed in implicit transaction, HashDB::OAUTOSYNC,
   * which means each updating operation is followed by implicit synchronization with the file
   * system.  The following may be added to both of the reader mode and the writer mode by
   * bitwise-or: HashDB::ONOLOCK, which means it opens the database file without file locking,
   * HashDB::OTRYLOCK, which means locking is performed without blocking, HashDB::ONOREPAIR,
   * which means the database file is not repaired implicitly even if file destruction is
   * detected.
   * @return true on success, or false on failure.
   * @note Every opened database must be closed by the HashDB::close method when it is no
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
    writer_ = false;
    autotran_ = false;
    autosync_ = false;
    reorg_ = false;
    trim_ = false;
    uint32_t fmode = File::OREADER;
    if (mode & OWRITER) {
      writer_ = true;
      fmode = File::OWRITER;
      if (mode & OCREATE) fmode |= File::OCREATE;
      if (mode & OTRUNCATE) fmode |= File::OTRUNCATE;
      if (mode & OAUTOTRAN) autotran_ = true;
      if (mode & OAUTOSYNC) autosync_ = true;
    }
    if (mode & ONOLOCK) fmode |= File::ONOLOCK;
    if (mode & OTRYLOCK) fmode |= File::OTRYLOCK;
    if (!file_.open(path, fmode, msiz_)) {
      const char* emsg = file_.error();
      Error::Code code = Error::SYSTEM;
      if (std::strstr(emsg, "(permission denied)") || std::strstr(emsg, "(directory)")) {
        code = Error::NOPERM;
      } else if (std::strstr(emsg, "(file not found)") || std::strstr(emsg, "(invalid path)")) {
        code = Error::NOREPOS;
      }
      set_error(_KCCODELINE_, code, emsg);
      return false;
    }
    if (file_.recovered()) report(_KCCODELINE_, Logger::WARN, "recovered by the WAL file");
    if ((mode & OWRITER) && file_.size() < 1) {
      calc_meta();
      libver_ = LIBVER;
      librev_ = LIBREV;
      fmtver_ = FMTVER;
      chksum_ = calc_checksum();
      lsiz_ = roff_;
      if (!file_.truncate(lsiz_)) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        file_.close();
        return false;
      }
      if (!dump_meta()) {
        file_.close();
        return false;
      }
      if (autosync_ && !File::synchronize_whole()) {
        set_error(_KCCODELINE_, Error::SYSTEM, "synchronizing the file system failed");
        file_.close();
        return false;
      }
    }
    if (!load_meta()) {
      file_.close();
      return false;
    }
    calc_meta();
    uint8_t chksum = calc_checksum();
    if (chksum != chksum_) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid module checksum");
      report(_KCCODELINE_, Logger::WARN, "saved=%02X calculated=%02X",
             (unsigned)chksum_, (unsigned)chksum);
      file_.close();
      return false;
    }
    if (((flags_ & FOPEN) || (flags_ & FFATAL)) && !(mode & ONOREPAIR) && !(mode & ONOLOCK)) {
      if (!reorganize_file(path)) {
        file_.close();
        return false;
      }
      if (!file_.close()) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        return false;
      }
      if (!file_.open(path, fmode, msiz_)) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        return false;
      }
      if (!load_meta()) {
        file_.close();
        return false;
      }
      calc_meta();
      reorg_ = true;
    }
    if (type_ == 0 || apow_ > MAXAPOW || fpow_ > MAXFPOW ||
        bnum_ < 1 || count_ < 0 || lsiz_ < roff_) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid meta data");
      report(_KCCODELINE_, Logger::WARN, "type=0x%02X apow=%d fpow=%d bnum=%lld count=%lld"
             " lsiz=%lld fsiz=%lld", (unsigned)type_, (int)apow_, (int)fpow_, (long long)bnum_,
             (long long)count_, (long long)lsiz_, (long long)file_.size());
      file_.close();
      return false;
    }
    if (file_.size() < lsiz_) {
      set_error(_KCCODELINE_, Error::BROKEN, "inconsistent file size");
      report(_KCCODELINE_, Logger::WARN, "lsiz=%lld fsiz=%lld",
             (long long)lsiz_, (long long)file_.size());
      file_.close();
      return false;
    }
    if (file_.size() != lsiz_ && !(mode & ONOREPAIR) && !(mode & ONOLOCK) && !trim_file(path)) {
      file_.close();
      return false;
    }
    if (mode & OWRITER) {
      if (!(flags_ & FOPEN) && !(flags_ & FFATAL) && !load_free_blocks()) {
        file_.close();
        return false;
      }
      if (!dump_empty_free_blocks()) {
        file_.close();
        return false;
      }
      if (!autotran_ && !set_flag(FOPEN, true)) {
        file_.close();
        return false;
      }
    }
    path_.append(path);
    omode_ = mode;
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
    report(_KCCODELINE_, Logger::DEBUG, "closing the database (path=%s)", path_.c_str());
    bool err = false;
    if (tran_ && !abort_transaction()) err = true;
    disable_cursors();
    if (writer_) {
      if (!dump_free_blocks()) err = true;
      if (!dump_meta()) err = true;
    }
    if (!file_.close()) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    fbp_.clear();
    omode_ = 0;
    path_.clear();
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
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    rlock_.lock_reader_all();
    bool err = false;
    if (!synchronize_impl(hard, proc, checker)) err = true;
    trigger_meta(MetaTrigger::SYNCHRONIZE, "synchronize");
    rlock_.unlock_all();
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
    if (proc && !proc->process(path_, count_, lsiz_)) {
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
    trhard_ = hard;
    if (!begin_transaction_impl()) {
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
    trhard_ = hard;
    if (!begin_transaction_impl()) {
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
    if (!file_.truncate(HEADSIZ)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    fbp_.clear();
    bool err = false;
    reorg_ = false;
    trim_ = false;
    flags_ = 0;
    flagopen_ = false;
    count_ = 0;
    lsiz_ = roff_;
    psiz_ = lsiz_;
    dfcur_ = roff_;
    std::memset(opaque_, 0, sizeof(opaque_));
    if (!file_.truncate(lsiz_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    if (!dump_meta()) err = true;
    if (!autotran_ && !set_flag(FOPEN, true)) err = true;
    trigger_meta(MetaTrigger::CLEAR, "clear");
    return true;
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
    return lsiz_;
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
    return path_;
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
    (*strmap)["type"] = strprintf("%u", (unsigned)TYPEHASH);
    (*strmap)["realtype"] = strprintf("%u", (unsigned)type_);
    (*strmap)["path"] = path_;
    (*strmap)["libver"] = strprintf("%u", libver_);
    (*strmap)["librev"] = strprintf("%u", librev_);
    (*strmap)["fmtver"] = strprintf("%u", fmtver_);
    (*strmap)["chksum"] = strprintf("%u", chksum_);
    (*strmap)["flags"] = strprintf("%u", flags_);
    (*strmap)["apow"] = strprintf("%u", apow_);
    (*strmap)["fpow"] = strprintf("%u", fpow_);
    (*strmap)["opts"] = strprintf("%u", opts_);
    (*strmap)["bnum"] = strprintf("%lld", (long long)bnum_);
    (*strmap)["msiz"] = strprintf("%lld", (long long)msiz_);
    (*strmap)["dfunit"] = strprintf("%lld", (long long)dfunit_);
    (*strmap)["frgcnt"] = strprintf("%lld", (long long)(frgcnt_ > 0 ? (int64_t)frgcnt_ : 0));
    (*strmap)["realsize"] = strprintf("%lld", (long long)file_.size());
    (*strmap)["recovered"] = strprintf("%d", file_.recovered());
    (*strmap)["reorganized"] = strprintf("%d", reorg_);
    (*strmap)["trimmed"] = strprintf("%d", trim_);
    if (strmap->count("opaque") > 0)
      (*strmap)["opaque"] = std::string(opaque_, sizeof(opaque_));
    if (strmap->count("fbpnum_used") > 0) {
      if (writer_) {
        (*strmap)["fbpnum_used"] = strprintf("%lld", (long long)fbp_.size());
      } else {
        if (!load_free_blocks()) return false;
        (*strmap)["fbpnum_used"] = strprintf("%lld", (long long)fbp_.size());
        fbp_.clear();
      }
    }
    if (strmap->count("bnum_used") > 0) {
      int64_t cnt = 0;
      for (int64_t i = 0; i < bnum_; i++) {
        if (get_bucket(i) > 0) cnt++;
      }
      (*strmap)["bnum_used"] = strprintf("%lld", (long long)cnt);
    }
    (*strmap)["count"] = strprintf("%lld", (long long)count_);
    (*strmap)["size"] = strprintf("%lld", (long long)lsiz_);
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
    if (!logger_) return;
    logger_->log(file, line, func, kind, message);
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
    logger_ = logger;
    logkinds_ = kinds;
    return true;
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
    if (apow_ > MAXAPOW) apow_ = MAXAPOW;
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
    if (fpow_ > MAXFPOW) fpow_ = MAXFPOW;
    return true;
  }
  /**
   * Set the optional features.
   * @param opts the optional features by bitwise-or: HashDB::TSMALL to use 32-bit addressing,
   * HashDB::TLINEAR to use linear collision chaining, HashDB::TCOMPRESS to compress each record.
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
    if (bnum_ > INT16MAX) bnum_ = nearbyprime(bnum_);
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
    msiz_ = msiz >= 0 ? msiz : DEFMSIZ;
    return true;
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
    dfunit_ = dfunit > 0 ? dfunit : 0;
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
    embcomp_ = comp;
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
    return opaque_;
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
    if (!writer_) {
      set_error(_KCCODELINE_, Error::NOPERM, "permission denied");
      return false;
    }
    bool err = false;
    if (!dump_opaque()) err = true;
    return !err;
  }
  /**
   * Perform defragmentation of the file.
   * @param step the number of steps.  If it is not more than 0, the whole region is defraged.
   * @return true on success, or false on failure.
   */
  bool defrag(int64_t step = 0) {
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
    bool err = false;
    if (step > 0) {
      if (!defrag_impl(step)) err = true;
    } else {
      dfcur_ = roff_;
      if (!defrag_impl(INT64MAX)) err = true;
    }
    frgcnt_ = 0;
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
    return flags_;
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
    if (!logger_ || !(kind & logkinds_)) return;
    std::string message;
    strprintf(&message, "%s: ", path_.empty() ? "-" : path_.c_str());
    va_list ap;
    va_start(ap, format);
    vstrprintf(&message, format, ap);
    va_end(ap);
    logger_->log(file, line, func, kind, message.c_str());
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
    if (!logger_ || !(kind & logkinds_)) return;
    std::string message;
    strprintf(&message, "%s: ", path_.empty() ? "-" : path_.c_str());
    vstrprintf(&message, format, ap);
    logger_->log(file, line, func, kind, message.c_str());
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
    if (!logger_) return;
    char* hex = hexencode(buf, size);
    report(file, line, func, kind, "%s=%s", name, hex);
    delete[] hex;
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
  /**
   * Set the database type.
   * @param type the database type.
   * @return true on success, or false on failure.
   */
  bool tune_type(int8_t type) {
    _assert_(true);
    ScopedRWLock lock(&mlock_, true);
    if (omode_ != 0) {
      set_error(_KCCODELINE_, Error::INVALID, "already opened");
      return false;
    }
    type_ = type;
    return true;
  }
  /**
   * Get the library version.
   * @return the library version, or 0 on failure.
   */
  uint8_t libver() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return libver_;
  }
  /**
   * Get the library revision.
   * @return the library revision, or 0 on failure.
   */
  uint8_t librev() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return librev_;
  }
  /**
   * Get the format version.
   * @return the format version, or 0 on failure.
   */
  uint8_t fmtver() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return fmtver_;
  }
  /**
   * Get the module checksum.
   * @return the module checksum, or 0 on failure.
   */
  uint8_t chksum() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return chksum_;
  }
  /**
   * Get the database type.
   * @return the database type, or 0 on failure.
   */
  uint8_t type() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return type_;
  }
  /**
   * Get the alignment power.
   * @return the alignment power, or 0 on failure.
   */
  uint8_t apow() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return apow_;
  }
  /**
   * Get the free block pool power.
   * @return the free block pool power, or 0 on failure.
   */
  uint8_t fpow() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return fpow_;
  }
  /**
   * Get the options.
   * @return the options, or 0 on failure.
   */
  uint8_t opts() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return opts_;
  }
  /**
   * Get the bucket number.
   * @return the bucket number, or 0 on failure.
   */
  int64_t bnum() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return bnum_;
  }
  /**
   * Get the size of the internal memory-mapped region.
   * @return the size of the internal memory-mapped region, or 0 on failure.
   */
  int64_t msiz() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return msiz_;
  }
  /**
   * Get the unit step number of auto defragmentation.
   * @return the unit step number of auto defragmentation, or 0 on failure.
   */
  int64_t dfunit() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return 0;
    }
    return dfunit_;
  }
  /**
   * Get the data compressor.
   * @return the data compressor, or NULL on failure.
   */
  Compressor* comp() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return NULL;
    }
    return comp_;
  }
  /**
   * Check whether the database was recovered or not.
   * @return true if recovered, or false if not.
   */
  bool recovered() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    return file_.recovered();
  }
  /**
   * Check whether the database was reorganized or not.
   * @return true if reorganized, or false if not.
   */
  bool reorganized() {
    _assert_(true);
    ScopedRWLock lock(&mlock_, false);
    if (omode_ == 0) {
      set_error(_KCCODELINE_, Error::INVALID, "not opened");
      return false;
    }
    return reorg_;
  }
 private:
  /**
   * Record data.
   */
  struct Record {
    int64_t off;                         ///< offset
    size_t rsiz;                         ///< whole size
    size_t psiz;                         ///< size of the padding
    size_t ksiz;                         ///< size of the key
    size_t vsiz;                         ///< size of the value
    int64_t left;                        ///< address of the left child record
    int64_t right;                       ///< address of the right child record
    const char* kbuf;                    ///< pointer to the key
    const char* vbuf;                    ///< pointer to the value
    int64_t boff;                        ///< offset of the body
    char* bbuf;                          ///< buffer of the body
  };
  /**
   * Free block data.
   */
  struct FreeBlock {
    int64_t off;                         ///< offset
    size_t rsiz;                         ///< record size
    /** comparing operator */
    bool operator <(const FreeBlock& obj) const {
      _assert_(true);
      if (rsiz < obj.rsiz) return true;
      if (rsiz == obj.rsiz && off > obj.off) return true;
      return false;
    }
  };
  /**
   * Comparator for free blocks.
   */
  struct FreeBlockComparator {
    /** comparing operator */
    bool operator ()(const FreeBlock& a, const FreeBlock& b) const {
      _assert_(true);
      return a.off < b.off;
    }
  };
  /**
   * Repeating visitor.
   */
  class Repeater : public Visitor {
   public:
    /** constructor */
    explicit Repeater(const char* vbuf, size_t vsiz) : vbuf_(vbuf), vsiz_(vsiz) {
      _assert_(vbuf);
    }
   private:
    /** visit a record */
    const char* visit_full(const char* kbuf, size_t ksiz,
                           const char* vbuf, size_t vsiz, size_t* sp) {
      _assert_(kbuf && ksiz <= MEMMAXSIZ && vbuf && vsiz <= MEMMAXSIZ && sp);
      *sp = vsiz_;
      return vbuf_;
    }
    const char* vbuf_;
    size_t vsiz_;
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
   * Accept a visitor to a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @param visitor a visitor object.
   * @param bidx the bucket index.
   * @param pivot the second hash value.
   @ @param isiter true for iterator use, or false for direct use.
   * @return true on success, or false on failure.
   */
  bool accept_impl(const char* kbuf, size_t ksiz, Visitor* visitor,
                   int64_t bidx, uint32_t pivot, bool isiter) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ && visitor && bidx >= 0);
    int64_t top = get_bucket(bidx);
    int64_t off = top;
    if (off < 0) return false;
    enum { DIREMPTY, DIRLEFT, DIRRIGHT, DIRMIXED } entdir = DIREMPTY;
    int64_t entoff = 0;
    Record rec;
    char rbuf[RECBUFSIZ];
    while (off > 0) {
      rec.off = off;
      if (!read_record(&rec, rbuf)) return false;
      if (rec.psiz == UINT16MAX) {
        set_error(_KCCODELINE_, Error::BROKEN, "free block in the chain");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec.off, (long long)file_.size());
        return false;
      }
      uint32_t tpivot = linear_ ? pivot : fold_hash(hash_record(rec.kbuf, rec.ksiz));
      if (pivot > tpivot) {
        delete[] rec.bbuf;
        off = rec.left;
        switch (entdir) {
          case DIREMPTY: entdir = DIRLEFT; break;
          case DIRRIGHT: entdir = DIRMIXED; break;
          default: break;
        }
        entoff = rec.off + sizeof(uint16_t);
      } else if (pivot < tpivot) {
        delete[] rec.bbuf;
        off = rec.right;
        switch (entdir) {
          case DIREMPTY: entdir = DIRRIGHT; break;
          case DIRLEFT: entdir = DIRMIXED; break;
          default: break;
        }
        entoff = rec.off + sizeof(uint16_t) + width_;
      } else {
        int32_t kcmp = compare_keys(kbuf, ksiz, rec.kbuf, rec.ksiz);
        if (linear_ && kcmp != 0) kcmp = 1;
        if (kcmp > 0) {
          delete[] rec.bbuf;
          off = rec.left;
          switch (entdir) {
            case DIREMPTY: entdir = DIRLEFT; break;
            case DIRRIGHT: entdir = DIRMIXED; break;
            default: break;
          }
          entoff = rec.off + sizeof(uint16_t);
        } else if (kcmp < 0) {
          delete[] rec.bbuf;
          off = rec.right;
          switch (entdir) {
            case DIREMPTY: entdir = DIRRIGHT; break;
            case DIRLEFT: entdir = DIRMIXED; break;
            default: break;
          }
          entoff = rec.off + sizeof(uint16_t) + width_;
        } else {
          if (!rec.vbuf && !read_record_body(&rec)) {
            delete[] rec.bbuf;
            return false;
          }
          const char* vbuf = rec.vbuf;
          size_t vsiz = rec.vsiz;
          char* zbuf = NULL;
          size_t zsiz = 0;
          if (comp_) {
            zbuf = comp_->decompress(vbuf, vsiz, &zsiz);
            if (!zbuf) {
              set_error(_KCCODELINE_, Error::SYSTEM, "data decompression failed");
              delete[] rec.bbuf;
              return false;
            }
            vbuf = zbuf;
            vsiz = zsiz;
          }
          vbuf = visitor->visit_full(kbuf, ksiz, vbuf, vsiz, &vsiz);
          delete[] zbuf;
          if (vbuf == Visitor::REMOVE) {
            bool atran = false;
            if (autotran_ && !tran_) {
              if (!begin_auto_transaction()) {
                delete[] rec.bbuf;
                return false;
              }
              atran = true;
            }
            if (!write_free_block(rec.off, rec.rsiz, rbuf)) {
              if (atran) abort_auto_transaction();
              delete[] rec.bbuf;
              return false;
            }
            insert_free_block(rec.off, rec.rsiz);
            frgcnt_ += 1;
            delete[] rec.bbuf;
            if (!cut_chain(&rec, rbuf, bidx, entoff)) {
              if (atran) abort_auto_transaction();
              return false;
            }
            count_ -= 1;
            if (atran) {
              if (!commit_auto_transaction()) return false;
            } else if (autosync_) {
              if (!synchronize_meta()) return false;
            }
          } else if (vbuf == Visitor::NOP) {
            delete[] rec.bbuf;
          } else {
            zbuf = NULL;
            zsiz = 0;
            if (comp_ && !isiter) {
              zbuf = comp_->compress(vbuf, vsiz, &zsiz);
              if (!zbuf) {
                set_error(_KCCODELINE_, Error::SYSTEM, "data compression failed");
                delete[] rec.bbuf;
                return false;
              }
              vbuf = zbuf;
              vsiz = zsiz;
            }
            bool atran = false;
            if (autotran_ && !tran_) {
              if (!begin_auto_transaction()) {
                delete[] zbuf;
                delete[] rec.bbuf;
                return false;
              }
              atran = true;
            }
            size_t rsiz = calc_record_size(rec.ksiz, vsiz);
            if (rsiz <= rec.rsiz) {
              rec.psiz = rec.rsiz - rsiz;
              rec.vsiz = vsiz;
              rec.vbuf = vbuf;
              if (!adjust_record(&rec) || !write_record(&rec, true)) {
                if (atran) abort_auto_transaction();
                delete[] zbuf;
                delete[] rec.bbuf;
                return false;
              }
              delete[] zbuf;
              delete[] rec.bbuf;
            } else {
              if (!write_free_block(rec.off, rec.rsiz, rbuf)) {
                if (atran) abort_auto_transaction();
                delete[] zbuf;
                delete[] rec.bbuf;
                return false;
              }
              insert_free_block(rec.off, rec.rsiz);
              frgcnt_ += 1;
              size_t psiz = calc_record_padding(rsiz);
              rec.rsiz = rsiz + psiz;
              rec.psiz = psiz;
              rec.vsiz = vsiz;
              rec.vbuf = vbuf;
              bool over = false;
              FreeBlock fb;
              if (!isiter && fetch_free_block(rec.rsiz, &fb)) {
                rec.off = fb.off;
                rec.rsiz = fb.rsiz;
                rec.psiz = rec.rsiz - rsiz;
                over = true;
                if (!adjust_record(&rec)) {
                  if (atran) abort_auto_transaction();
                  delete[] zbuf;
                  delete[] rec.bbuf;
                  return false;
                }
              } else {
                rec.off = lsiz_.add(rec.rsiz);
              }
              if (!write_record(&rec, over)) {
                if (atran) abort_auto_transaction();
                delete[] zbuf;
                delete[] rec.bbuf;
                return false;
              }
              if (!over) psiz_.secure_least(rec.off + rec.rsiz);
              delete[] zbuf;
              delete[] rec.bbuf;
              if (entoff > 0) {
                if (!set_chain(entoff, rec.off)) {
                  if (atran) abort_auto_transaction();
                  return false;
                }
              } else {
                if (!set_bucket(bidx, rec.off)) {
                  if (atran) abort_auto_transaction();
                  return false;
                }
              }
            }
            if (atran) {
              if (!commit_auto_transaction()) return false;
            } else if (autosync_) {
              if (!synchronize_meta()) return false;
            }
          }
          return true;
        }
      }
    }
    size_t vsiz;
    const char* vbuf = visitor->visit_empty(kbuf, ksiz, &vsiz);
    if (vbuf != Visitor::NOP && vbuf != Visitor::REMOVE) {
      char* zbuf = NULL;
      size_t zsiz = 0;
      if (comp_) {
        zbuf = comp_->compress(vbuf, vsiz, &zsiz);
        if (!zbuf) {
          set_error(_KCCODELINE_, Error::SYSTEM, "data compression failed");
          return false;
        }
        vbuf = zbuf;
        vsiz = zsiz;
      }
      bool atran = false;
      if (autotran_ && !tran_) {
        if (!begin_auto_transaction()) {
          delete[] zbuf;
          return false;
        }
        atran = true;
      }
      size_t rsiz = calc_record_size(ksiz, vsiz);
      size_t psiz = calc_record_padding(rsiz);
      rec.rsiz = rsiz + psiz;
      rec.psiz = psiz;
      rec.ksiz = ksiz;
      rec.vsiz = vsiz;
      switch (entdir) {
        default: {
          rec.left = 0;
          rec.right = 0;
          break;
        }
        case DIRLEFT: {
          if (linear_) {
            rec.left = top;
            rec.right = 0;
          } else {
            rec.left = 0;
            rec.right = top;
          }
          break;
        }
        case DIRRIGHT: {
          rec.left = top;
          rec.right = 0;
          break;
        }
      }
      rec.kbuf = kbuf;
      rec.vbuf = vbuf;
      bool over = false;
      FreeBlock fb;
      if (fetch_free_block(rec.rsiz, &fb)) {
        rec.off = fb.off;
        rec.rsiz = fb.rsiz;
        rec.psiz = rec.rsiz - rsiz;
        over = true;
        if (!adjust_record(&rec)) {
          if (atran) abort_auto_transaction();
          delete[] zbuf;
          return false;
        }
      } else {
        rec.off = lsiz_.add(rec.rsiz);
      }
      if (!write_record(&rec, over)) {
        if (atran) abort_auto_transaction();
        delete[] zbuf;
        return false;
      }
      if (!over) psiz_.secure_least(rec.off + rec.rsiz);
      delete[] zbuf;
      if (entoff < 1 || entdir == DIRLEFT || entdir == DIRRIGHT) {
        if (!set_bucket(bidx, rec.off)) {
          if (atran) abort_auto_transaction();
          return false;
        }
      } else {
        if (!set_chain(entoff, rec.off)) {
          if (atran) abort_auto_transaction();
          return false;
        }
      }
      count_ += 1;
      if (atran) {
        if (!commit_auto_transaction()) return false;
      } else if (autosync_) {
        if (!synchronize_meta()) return false;
      }
    }
    return true;
  }
  /**
   * Iterate to accept a visitor for each record.
   * @param visitor a visitor object.
   * @param checker a progress checker object.
   * @return true on success, or false on failure.
   */
  bool iterate_impl(Visitor* visitor, ProgressChecker* checker) {
    _assert_(visitor);
    int64_t allcnt = count_;
    if (checker && !checker->check("iterate", "beginning", 0, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    int64_t off = roff_;
    int64_t end = lsiz_;
    Record rec;
    char rbuf[RECBUFSIZ];
    int64_t curcnt = 0;
    while (off > 0 && off < end) {
      rec.off = off;
      if (!read_record(&rec, rbuf)) return false;
      if (rec.psiz == UINT16MAX) {
        off += rec.rsiz;
      } else {
        if (!rec.vbuf && !read_record_body(&rec)) {
          delete[] rec.bbuf;
          return false;
        }
        const char* vbuf = rec.vbuf;
        size_t vsiz = rec.vsiz;
        char* zbuf = NULL;
        size_t zsiz = 0;
        if (comp_) {
          zbuf = comp_->decompress(vbuf, vsiz, &zsiz);
          if (!zbuf) {
            set_error(_KCCODELINE_, Error::SYSTEM, "data decompression failed");
            delete[] rec.bbuf;
            return false;
          }
          vbuf = zbuf;
          vsiz = zsiz;
        }
        vbuf = visitor->visit_full(rec.kbuf, rec.ksiz, vbuf, vsiz, &vsiz);
        delete[] zbuf;
        if (vbuf == Visitor::REMOVE) {
          uint64_t hash = hash_record(rec.kbuf, rec.ksiz);
          uint32_t pivot = fold_hash(hash);
          int64_t bidx = hash % bnum_;
          Repeater repeater(Visitor::REMOVE, 0);
          if (!accept_impl(rec.kbuf, rec.ksiz, &repeater, bidx, pivot, true)) {
            delete[] rec.bbuf;
            return false;
          }
          delete[] rec.bbuf;
        } else if (vbuf == Visitor::NOP) {
          delete[] rec.bbuf;
        } else {
          zbuf = NULL;
          zsiz = 0;
          if (comp_) {
            zbuf = comp_->compress(vbuf, vsiz, &zsiz);
            if (!zbuf) {
              set_error(_KCCODELINE_, Error::SYSTEM, "data compression failed");
              delete[] rec.bbuf;
              return false;
            }
            vbuf = zbuf;
            vsiz = zsiz;
          }
          size_t rsiz = calc_record_size(rec.ksiz, vsiz);
          if (rsiz <= rec.rsiz) {
            rec.psiz = rec.rsiz - rsiz;
            rec.vsiz = vsiz;
            rec.vbuf = vbuf;
            if (!adjust_record(&rec) || !write_record(&rec, true)) {
              delete[] zbuf;
              delete[] rec.bbuf;
              return false;
            }
            delete[] zbuf;
            delete[] rec.bbuf;
          } else {
            uint64_t hash = hash_record(rec.kbuf, rec.ksiz);
            uint32_t pivot = fold_hash(hash);
            int64_t bidx = hash % bnum_;
            Repeater repeater(vbuf, vsiz);
            if (!accept_impl(rec.kbuf, rec.ksiz, &repeater, bidx, pivot, true)) {
              delete[] zbuf;
              delete[] rec.bbuf;
              return false;
            }
            delete[] zbuf;
            delete[] rec.bbuf;
          }
        }
        off += rec.rsiz;
        curcnt++;
        if (checker && !checker->check("iterate", "processing", curcnt, allcnt)) {
          set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
          return false;
        }
      }
    }
    if (checker && !checker->check("iterate", "ending", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    return true;
  }
  /**
   * Scan each record in parallel.
   * @param visitor a visitor object.
   * @param thnum the number of worker threads.
   * @param checker a progress checker object.
   * @return true on success, or false on failure.
   */
  bool scan_parallel_impl(Visitor *visitor, size_t thnum, ProgressChecker* checker) {
    _assert_(visitor && thnum <= MEMMAXSIZ);
    int64_t allcnt = count_;
    if (checker && !checker->check("scan_parallel", "beginning", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      return false;
    }
    bool err = false;
    std::vector<int64_t> offs;
    int64_t bnum = bnum_;
    size_t cap = (thnum + 1) * INT8MAX;
    for (int64_t bidx = 0; bidx < bnum; bidx++) {
      int64_t off = get_bucket(bidx);
      if (off > 0) {
        offs.push_back(off);
        if (offs.size() >= cap) break;
      }
    }
    if (!offs.empty()) {
      std::sort(offs.begin(), offs.end());
      if (thnum > offs.size()) thnum = offs.size();
      class ThreadImpl : public Thread {
       public:
        explicit ThreadImpl() :
            db_(NULL), visitor_(NULL), checker_(NULL), allcnt_(0),
            begoff_(0), endoff_(0), error_() {}
        void init(HashDB* db, Visitor* visitor, ProgressChecker* checker, int64_t allcnt,
                  int64_t begoff, int64_t endoff) {
          db_ = db;
          visitor_ = visitor;
          checker_ = checker;
          allcnt_ = allcnt;
          begoff_ = begoff;
          endoff_ = endoff;
        }
        const Error& error() {
          return error_;
        }
       private:
        void run() {
          HashDB* db = db_;
          Visitor* visitor = visitor_;
          ProgressChecker* checker = checker_;
          int64_t off = begoff_;
          int64_t end = endoff_;
          int64_t allcnt = allcnt_;
          Compressor* comp = db->comp_;
          Record rec;
          char rbuf[RECBUFSIZ];
          while (off > 0 && off < end) {
            rec.off = off;
            if (!db->read_record(&rec, rbuf)) {
              error_ = db->error();
              break;
            }
            if (rec.psiz == UINT16MAX) {
              off += rec.rsiz;
            } else {
              if (!rec.vbuf && !db->read_record_body(&rec)) {
                delete[] rec.bbuf;
                error_ = db->error();
                break;
              }
              const char* vbuf = rec.vbuf;
              size_t vsiz = rec.vsiz;
              char* zbuf = NULL;
              size_t zsiz = 0;
              if (comp) {
                zbuf = comp->decompress(vbuf, vsiz, &zsiz);
                if (!zbuf) {
                  db->set_error(_KCCODELINE_, Error::SYSTEM, "data decompression failed");
                  delete[] rec.bbuf;
                  error_ = db->error();
                  break;
                }
                vbuf = zbuf;
                vsiz = zsiz;
              }
              visitor->visit_full(rec.kbuf, rec.ksiz, vbuf, vsiz, &vsiz);
              delete[] zbuf;
              delete[] rec.bbuf;
              off += rec.rsiz;
              if (checker && !checker->check("scan_parallel", "processing", -1, allcnt)) {
                db->set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
                error_ = db->error();
                break;
              }
            }
          }
        }
        HashDB* db_;
        Visitor* visitor_;
        ProgressChecker* checker_;
        int64_t allcnt_;
        int64_t begoff_;
        int64_t endoff_;
        Error error_;
      };
      ThreadImpl* threads = new ThreadImpl[thnum];
      double range = (double)offs.size() / thnum;
      for (size_t i = 0; i < thnum; i++) {
        int64_t cidx = i * range;
        int64_t nidx = (i + 1) * range;
        int64_t begoff = i < 1 ? roff_ : offs[cidx];
        int64_t endoff = i < thnum - 1 ? offs[nidx] : (int64_t)lsiz_;
        ThreadImpl* thread = threads + i;
        thread->init(this, visitor, checker, allcnt, begoff, endoff);
        thread->start();
      }
      for (size_t i = 0; i < thnum; i++) {
        ThreadImpl* thread = threads + i;
        thread->join();
        if (thread->error() != Error::SUCCESS) {
          *error_ = thread->error();
          err = true;
        }
      }
      delete[] threads;
    }
    if (checker && !checker->check("scan_parallel", "ending", -1, allcnt)) {
      set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
      err = true;
    }
    return !err;
  }
  /**
   * Synchronize updated contents with the file and the device.
   * @param hard true for physical synchronization with the device, or false for logical
   * synchronization with the file system.
   * @param proc a postprocessor object.
   * @param checker a progress checker object.
   * @return true on success, or false on failure.
   */
  bool synchronize_impl(bool hard, FileProcessor* proc, ProgressChecker* checker) {
    _assert_(true);
    bool err = false;
    if (writer_) {
      if (checker && !checker->check("synchronize", "dumping the free blocks", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (hard && !dump_free_blocks()) err = true;
      if (checker && !checker->check("synchronize", "dumping the meta data", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (!dump_meta()) err = true;
      if (checker && !checker->check("synchronize", "synchronizing the file", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (!file_.synchronize(hard)) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        err = true;
      }
    }
    if (proc) {
      if (checker && !checker->check("synchronize", "running the post processor", -1, -1)) {
        set_error(_KCCODELINE_, Error::LOGIC, "checker failed");
        return false;
      }
      if (!proc->process(path_, count_, lsiz_)) {
        set_error(_KCCODELINE_, Error::LOGIC, "postprocessing failed");
        err = true;
      }
    }
    if (writer_ && !autotran_ && !set_flag(FOPEN, true)) err = true;
    return !err;
  }
  /**
   * Synchronize meta data with the file and the device.
   * @return true on success, or false on failure.
   */
  bool synchronize_meta() {
    _assert_(true);
    ScopedMutex lock(&flock_);
    bool err = false;
    if (!dump_meta()) err = true;
    if (!file_.synchronize(true)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    return !err;
  }
  /**
   * Perform defragmentation.
   * @param step the number of steps.
   * @return true on success, or false on failure.
   */
  bool defrag_impl(int64_t step) {
    _assert_(step >= 0);
    int64_t end = lsiz_;
    Record rec;
    char rbuf[RECBUFSIZ];
    while (true) {
      if (dfcur_ >= end) {
        dfcur_ = roff_;
        return true;
      }
      if (step-- < 1) return true;
      rec.off = dfcur_;
      if (!read_record(&rec, rbuf)) return false;
      if (rec.psiz == UINT16MAX) break;
      delete[] rec.bbuf;
      dfcur_ += rec.rsiz;
    }
    bool atran = false;
    if (autotran_ && !tran_) {
      if (!begin_auto_transaction()) return false;
      atran = true;
    }
    int64_t base = dfcur_;
    int64_t dest = base;
    dfcur_ += rec.rsiz;
    step++;
    while (step-- > 0 && dfcur_ < end) {
      rec.off = dfcur_;
      if (!read_record(&rec, rbuf)) {
        if (atran) abort_auto_transaction();
        return false;
      }
      escape_cursors(rec.off, dest);
      dfcur_ += rec.rsiz;
      if (rec.psiz != UINT16MAX) {
        if (!rec.vbuf && !read_record_body(&rec)) {
          if (atran) abort_auto_transaction();
          delete[] rec.bbuf;
          return false;
        }
        if (rec.psiz >= align_) {
          size_t diff = rec.psiz - rec.psiz % align_;
          rec.psiz -= diff;
          rec.rsiz -= diff;
        }
        if (!shift_record(&rec, dest)) {
          if (atran) abort_auto_transaction();
          delete[] rec.bbuf;
          return false;
        }
        delete[] rec.bbuf;
        dest += rec.rsiz;
      }
    }
    trim_free_blocks(base, dfcur_);
    if (dfcur_ >= end) {
      lsiz_ = dest;
      psiz_ = lsiz_;
      if (!file_.truncate(lsiz_)) {
        if (atran) abort_auto_transaction();
        return false;
      }
      trim_cursors();
      dfcur_ = roff_;
    } else {
      size_t rsiz = dfcur_ - dest;
      if (!write_free_block(dest, rsiz, rbuf)) {
        if (atran) abort_auto_transaction();
        return false;
      }
      insert_free_block(dest, rsiz);
      dfcur_ = dest;
    }
    if (atran) {
      if (!commit_auto_transaction()) return false;
    } else if (autosync_) {
      if (!synchronize_meta()) return false;
    }
    return true;
  }
  /**
   * Calculate meta data with saved ones.
   */
  void calc_meta() {
    _assert_(true);
    align_ = 1 << apow_;
    fbpnum_ = fpow_ > 0 ? 1 << fpow_ : 0;
    width_ = (opts_ & TSMALL) ? sizeof(uint32_t) : sizeof(uint32_t) + 2;
    linear_ = (opts_ & TLINEAR) ? true : false;
    comp_ = (opts_ & TCOMPRESS) ? embcomp_ : NULL;
    rhsiz_ = sizeof(uint16_t) + sizeof(uint8_t) * 2;
    rhsiz_ += linear_ ? width_ : width_ * 2;
    boff_ = HEADSIZ + FBPWIDTH * fbpnum_;
    if (fbpnum_ > 0) boff_ += width_ * 2 + sizeof(uint8_t) * 2;
    roff_ = boff_ + width_ * bnum_;
    int64_t rem = roff_ % align_;
    if (rem > 0) roff_ += align_ - rem;
    dfcur_ = roff_;
    frgcnt_ = 0;
    tran_ = false;
  }
  /**
   * Calculate the module checksum.
   * @return the module checksum.
   */
  uint8_t calc_checksum() {
    _assert_(true);
    const char* kbuf = KCHDBCHKSUMSEED;
    size_t ksiz = sizeof(KCHDBCHKSUMSEED) - 1;
    char* zbuf = NULL;
    size_t zsiz = 0;
    if (comp_) {
      zbuf = comp_->compress(kbuf, ksiz, &zsiz);
      if (!zbuf) return 0;
      kbuf = zbuf;
      ksiz = zsiz;
    }
    uint32_t hash = fold_hash(hash_record(kbuf, ksiz));
    delete[] zbuf;
    return (hash >> 24) ^ (hash >> 16) ^ (hash >> 8) ^ (hash >> 0);
  }
  /**
   * Dump the meta data into the file.
   * @return true on success, or false on failure.
   */
  bool dump_meta() {
    _assert_(true);
    char head[HEADSIZ];
    std::memset(head, 0, sizeof(head));
    std::memcpy(head, KCHDBMAGICDATA, sizeof(KCHDBMAGICDATA));
    std::memcpy(head + MOFFLIBVER, &libver_, sizeof(libver_));
    std::memcpy(head + MOFFLIBREV, &librev_, sizeof(librev_));
    std::memcpy(head + MOFFFMTVER, &fmtver_, sizeof(fmtver_));
    std::memcpy(head + MOFFCHKSUM, &chksum_, sizeof(chksum_));
    std::memcpy(head + MOFFTYPE, &type_, sizeof(type_));
    std::memcpy(head + MOFFAPOW, &apow_, sizeof(apow_));
    std::memcpy(head + MOFFFPOW, &fpow_, sizeof(fpow_));
    std::memcpy(head + MOFFOPTS, &opts_, sizeof(opts_));
    uint64_t num = hton64(bnum_);
    std::memcpy(head + MOFFBNUM, &num, sizeof(num));
    if (!flagopen_) flags_ &= ~FOPEN;
    std::memcpy(head + MOFFFLAGS, &flags_, sizeof(flags_));
    num = hton64(count_);
    std::memcpy(head + MOFFCOUNT, &num, sizeof(num));
    num = hton64(lsiz_);
    std::memcpy(head + MOFFSIZE, &num, sizeof(num));
    std::memcpy(head + MOFFOPAQUE, opaque_, sizeof(opaque_));
    if (!file_.write(0, head, sizeof(head))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    trcount_ = count_;
    trsize_ = lsiz_;
    return true;
  }
  /**
   * Dump the meta data into the file.
   * @return true on success, or false on failure.
   */
  bool dump_auto_meta() {
    _assert_(true);
    const int64_t hsiz = MOFFOPAQUE - MOFFCOUNT;
    char head[hsiz];
    std::memset(head, 0, hsiz);
    uint64_t num = hton64(count_);
    std::memcpy(head, &num, sizeof(num));
    num = hton64(lsiz_);
    std::memcpy(head + MOFFSIZE - MOFFCOUNT, &num, sizeof(num));
    if (!file_.write_fast(MOFFCOUNT, head, sizeof(head))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    trcount_ = count_;
    trsize_ = lsiz_;
    return true;
  }
  /**
   * Dump the opaque data into the file.
   * @return true on success, or false on failure.
   */
  bool dump_opaque() {
    _assert_(true);
    if (!file_.write_fast(MOFFOPAQUE, opaque_, sizeof(opaque_))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    return true;
  }
  /**
   * Load the meta data from the file.
   * @return true on success, or false on failure.
   */
  bool load_meta() {
    _assert_(true);
    char head[HEADSIZ];
    if (file_.size() < (int64_t)sizeof(head)) {
      set_error(_KCCODELINE_, Error::INVALID, "missing magic data of the file");
      return false;
    }
    if (!file_.read(0, head, sizeof(head))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)0, (long long)file_.size());
      return false;
    }
    if (std::memcmp(head, KCHDBMAGICDATA, sizeof(KCHDBMAGICDATA))) {
      set_error(_KCCODELINE_, Error::INVALID, "invalid magic data of the file");
      return false;
    }
    std::memcpy(&libver_, head + MOFFLIBVER, sizeof(libver_));
    std::memcpy(&librev_, head + MOFFLIBREV, sizeof(librev_));
    std::memcpy(&fmtver_, head + MOFFFMTVER, sizeof(fmtver_));
    std::memcpy(&chksum_, head + MOFFCHKSUM, sizeof(chksum_));
    std::memcpy(&type_, head + MOFFTYPE, sizeof(type_));
    std::memcpy(&apow_, head + MOFFAPOW, sizeof(apow_));
    std::memcpy(&fpow_, head + MOFFFPOW, sizeof(fpow_));
    std::memcpy(&opts_, head + MOFFOPTS, sizeof(opts_));
    uint64_t num;
    std::memcpy(&num, head + MOFFBNUM, sizeof(num));
    bnum_ = ntoh64(num);
    std::memcpy(&flags_, head + MOFFFLAGS, sizeof(flags_));
    flagopen_ = flags_ & FOPEN;
    std::memcpy(&num, head + MOFFCOUNT, sizeof(num));
    count_ = ntoh64(num);
    std::memcpy(&num, head + MOFFSIZE, sizeof(num));
    lsiz_ = ntoh64(num);
    psiz_ = lsiz_;
    std::memcpy(opaque_, head + MOFFOPAQUE, sizeof(opaque_));
    trcount_ = count_;
    trsize_ = lsiz_;
    return true;
  }
  /**
   * Set a status flag.
   * @param flag the flag kind.
   * @param sign whether to set or unset.
   * @return true on success, or false on failure.
   */
  bool set_flag(uint8_t flag, bool sign) {
    _assert_(true);
    uint8_t flags;
    if (!file_.read(MOFFFLAGS, &flags, sizeof(flags))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)MOFFFLAGS, (long long)file_.size());
      return false;
    }
    if (sign) {
      flags |= flag;
    } else {
      flags &= ~flag;
    }
    if (!file_.write(MOFFFLAGS, &flags, sizeof(flags))) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    flags_ = flags;
    return true;
  }
  /**
   * Reorganize the whole file.
   * @param path the path of the database file.
   * @return true on success, or false on failure.
   */
  bool reorganize_file(const std::string& path) {
    _assert_(true);
    bool err = false;
    HashDB db;
    db.tune_type(type_);
    db.tune_alignment(apow_);
    db.tune_fbp(fpow_);
    db.tune_options(opts_);
    db.tune_buckets(bnum_);
    db.tune_map(msiz_);
    if (embcomp_) db.tune_compressor(embcomp_);
    const std::string& npath = path + File::EXTCHR + KCHDBTMPPATHEXT;
    if (db.open(npath, OWRITER | OCREATE | OTRUNCATE)) {
      report(_KCCODELINE_, Logger::WARN, "reorganizing the database");
      lsiz_ = file_.size();
      psiz_ = lsiz_;
      if (copy_records(&db)) {
        if (db.close()) {
          if (!File::rename(npath, path)) {
            set_error(_KCCODELINE_, Error::SYSTEM, "renaming the destination failed");
            err = true;
          }
        } else {
          set_error(_KCCODELINE_, db.error().code(), "closing the destination failed");
          err = true;
        }
      } else {
        set_error(_KCCODELINE_, db.error().code(), "record copying failed");
        err = true;
      }
      File::remove(npath);
    } else {
      set_error(_KCCODELINE_, db.error().code(), "opening the destination failed");
      err = true;
    }
    return !err;
  }
  /**
   * Copy all records to another database.
   * @param dest the destination database.
   * @return true on success, or false on failure.
   */
  bool copy_records(HashDB* dest) {
    _assert_(dest);
    Logger* logger = logger_;
    logger_ = NULL;
    int64_t off = roff_;
    int64_t end = psiz_;
    Record rec, nrec;
    char rbuf[RECBUFSIZ], nbuf[RECBUFSIZ];
    while (off > 0 && off < end) {
      rec.off = off;
      if (!read_record(&rec, rbuf)) {
        int64_t checkend = off + SLVGWIDTH;
        if (checkend > end - (int64_t)rhsiz_) checkend = end - rhsiz_;
        bool hit = false;
        for (off += rhsiz_; off < checkend; off++) {
          rec.off = off;
          if (!read_record(&rec, rbuf)) continue;
          if ((int64_t)rec.rsiz > SLVGWIDTH || rec.off + (int64_t)rec.rsiz >= checkend) {
            delete[] rec.bbuf;
            continue;
          }
          if (rec.psiz != UINT16MAX && !rec.vbuf && !read_record_body(&rec)) {
            delete[] rec.bbuf;
            continue;
          }
          delete[] rec.bbuf;
          nrec.off = off + rec.rsiz;
          if (!read_record(&nrec, nbuf)) continue;
          if ((int64_t)nrec.rsiz > SLVGWIDTH || nrec.off + (int64_t)nrec.rsiz >= checkend) {
            delete[] nrec.bbuf;
            continue;
          }
          if (nrec.psiz != UINT16MAX && !nrec.vbuf && !read_record_body(&nrec)) {
            delete[] nrec.bbuf;
            continue;
          }
          delete[] nrec.bbuf;
          hit = true;
          break;
        }
        if (!hit || !read_record(&rec, rbuf)) break;
      }
      if (rec.psiz == UINT16MAX) {
        off += rec.rsiz;
        continue;
      }
      if (!rec.vbuf && !read_record_body(&rec)) {
        delete[] rec.bbuf;
        bool hit = false;
        if (rec.rsiz <= MEMMAXSIZ && off + (int64_t)rec.rsiz < end) {
          nrec.off = off + rec.rsiz;
          if (read_record(&nrec, nbuf)) {
            if (nrec.rsiz > MEMMAXSIZ || nrec.off + (int64_t)nrec.rsiz >= end) {
              delete[] nrec.bbuf;
            } else if (nrec.psiz != UINT16MAX && !nrec.vbuf && !read_record_body(&nrec)) {
              delete[] nrec.bbuf;
            } else {
              delete[] nrec.bbuf;
              hit = true;
            }
          }
        }
        if (hit) {
          off += rec.rsiz;
          continue;
        } else {
          break;
        }
      }
      const char* vbuf = rec.vbuf;
      size_t vsiz = rec.vsiz;
      char* zbuf = NULL;
      size_t zsiz = 0;
      if (comp_) {
        zbuf = comp_->decompress(vbuf, vsiz, &zsiz);
        if (!zbuf) {
          delete[] rec.bbuf;
          off += rec.rsiz;
          continue;
        }
        vbuf = zbuf;
        vsiz = zsiz;
      }
      if (!dest->set(rec.kbuf, rec.ksiz, vbuf, vsiz)) {
        delete[] zbuf;
        delete[] rec.bbuf;
        break;
      }
      delete[] zbuf;
      delete[] rec.bbuf;
      off += rec.rsiz;
    }
    logger_ = logger;
    return true;
  }
  /**
   * Trim the file size.
   * @param path the path of the database file.
   * @return true on success, or false on failure.
   */
  bool trim_file(const std::string& path) {
    _assert_(true);
    bool err = false;
    report(_KCCODELINE_, Logger::WARN, "trimming the database");
    File* dest = writer_ ? &file_ : new File();
    if (dest == &file_ || dest->open(path, File::OWRITER | File::ONOLOCK, 0)) {
      if (!dest->truncate(lsiz_)) {
        set_error(_KCCODELINE_, Error::SYSTEM, dest->error());
        err = true;
      }
      if (dest != &file_) {
        if (!dest->close()) {
          set_error(_KCCODELINE_, Error::SYSTEM, dest->error());
          err = true;
        }
        if (!file_.refresh()) {
          set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
          err = true;
        }
      }
      trim_ = true;
    } else {
      set_error(_KCCODELINE_, Error::SYSTEM, dest->error());
      err = true;
    }
    if (dest != &file_) delete dest;
    return !err;
  }
  /**
   * Get the hash value of a record.
   * @param kbuf the pointer to the key region.
   * @param ksiz the size of the key region.
   * @return the hash value.
   */
  uint64_t hash_record(const char* kbuf, size_t ksiz) {
    _assert_(kbuf && ksiz <= MEMMAXSIZ);
    return hashmurmur(kbuf, ksiz);
  }
  /**
   * Fold a hash value into a small number.
   * @param hash the hash number.
   * @return the result number.
   */
  uint32_t fold_hash(uint64_t hash) {
    _assert_(true);
    return (((hash & 0xffff000000000000ULL) >> 48) | ((hash & 0x0000ffff00000000ULL) >> 16)) ^
        (((hash & 0x000000000000ffffULL) << 16) | ((hash & 0x00000000ffff0000ULL) >> 16));
  }
  /**
   * Compare two keys in lexical order.
   * @param abuf one key.
   * @param asiz the size of the one key.
   * @param bbuf the other key.
   * @param bsiz the size of the other key.
   * @return positive if the former is big, or negative if the latter is big, or 0 if both are
   * equivalent.
   */
  int32_t compare_keys(const char* abuf, size_t asiz, const char* bbuf, size_t bsiz) {
    _assert_(abuf && bbuf);
    if (asiz != bsiz) return (int32_t)asiz - (int32_t)bsiz;
    return std::memcmp(abuf, bbuf, asiz);
  }
  /**
   * Set an address into a bucket.
   * @param bidx the index of the bucket.
   * @param off the address.
   * @return true on success, or false on failure.
   */
  bool set_bucket(int64_t bidx, int64_t off) {
    _assert_(bidx >= 0 && off >= 0);
    char buf[sizeof(uint64_t)];
    writefixnum(buf, off >> apow_, width_);
    if (!file_.write_fast(boff_ + bidx * width_, buf, width_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    return true;
  }
  /**
   * Get an address from a bucket.
   * @param bidx the index of the bucket.
   * @return the address, or -1 on failure.
   */
  int64_t get_bucket(int64_t bidx) {
    _assert_(bidx >= 0);
    char buf[sizeof(uint64_t)];
    if (!file_.read_fast(boff_ + bidx * width_, buf, width_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)boff_ + bidx * width_, (long long)file_.size());
      return -1;
    }
    return readfixnum(buf, width_) << apow_;
  }
  /**
   * Set an address into a chain slot.
   * @param entoff the address of the chain slot.
   * @param off the destination address.
   * @return true on success, or false on failure.
   */
  bool set_chain(int64_t entoff, int64_t off) {
    _assert_(entoff >= 0 && off >= 0);
    char buf[sizeof(uint64_t)];
    writefixnum(buf, off >> apow_, width_);
    if (!file_.write_fast(entoff, buf, width_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    return true;
  }
  /**
   * Read a record from the file.
   * @param rec the record structure.
   * @param rbuf the working buffer.
   * @return true on success, or false on failure.
   */
  bool read_record(Record* rec, char* rbuf) {
    _assert_(rec && rbuf);
    if (rec->off < roff_) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid record offset");
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)rec->off, (long long)file_.size());
      return false;
    }
    size_t rsiz = psiz_ - rec->off;
    if (rsiz > RECBUFSIZ) {
      rsiz = RECBUFSIZ;
    } else {
      if (rsiz < rhsiz_) {
        set_error(_KCCODELINE_, Error::BROKEN, "too short record region");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
        return false;
      }
      rsiz = rhsiz_;
    }
    if (!file_.read_fast(rec->off, rbuf, rsiz)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
             (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
      return false;
    }
    const char* rp = rbuf;
    uint16_t snum;
    if (*(uint8_t*)rp == RECMAGIC) {
      ((uint8_t*)&snum)[0] = 0;
      ((uint8_t*)&snum)[1] = *(uint8_t*)(rp + 1);
    } else if (*(uint8_t*)rp >= 0x80) {
      if (*(uint8_t*)(rp++) != FBMAGIC || *(uint8_t*)(rp++) != FBMAGIC) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid magic data of a free block");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
        report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
        return false;
      }
      rec->rsiz = readfixnum(rp, width_) << apow_;
      rp += width_;
      if (*(uint8_t*)(rp++) != PADMAGIC || *(uint8_t*)(rp++) != PADMAGIC) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid magic data of a free block");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
        report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
        return false;
      }
      if (rec->rsiz < rhsiz_) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid size of a free block");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
        report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
        return false;
      }
      rec->psiz = UINT16MAX;
      rec->ksiz = 0;
      rec->vsiz = 0;
      rec->left = 0;
      rec->right = 0;
      rec->kbuf = NULL;
      rec->vbuf = NULL;
      rec->boff = 0;
      rec->bbuf = NULL;
      return true;
    } else if (*rp == 0) {
      set_error(_KCCODELINE_, Error::BROKEN, "nullified region");
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
             (long long)psiz_, (long long)rec->off, (long long)rsiz, (long long)file_.size());
      report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
      return false;
    } else {
      std::memcpy(&snum, rp, sizeof(snum));
    }
    rp += sizeof(snum);
    rsiz -= sizeof(snum);
    rec->psiz = ntoh16(snum);
    rec->left = readfixnum(rp, width_) << apow_;
    rp += width_;
    rsiz -= width_;
    if (linear_) {
      rec->right = 0;
    } else {
      rec->right = readfixnum(rp, width_) << apow_;
      rp += width_;
      rsiz -= width_;
    }
    uint64_t num;
    size_t step = readvarnum(rp, rsiz, &num);
    if (step < 1) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid key length");
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld snum=%04X",
             (long long)psiz_, (long long)rec->off, (long long)rsiz,
             (long long)file_.size(), snum);
      report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
      return false;
    }
    rec->ksiz = num;
    rp += step;
    rsiz -= step;
    step = readvarnum(rp, rsiz, &num);
    if (step < 1) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid value length");
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld snum=%04X",
             (long long)psiz_, (long long)rec->off, (long long)rsiz,
             (long long)file_.size(), snum);
      report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
      return false;
    }
    rec->vsiz = num;
    rp += step;
    rsiz -= step;
    size_t hsiz = rp - rbuf;
    rec->rsiz = hsiz + rec->ksiz + rec->vsiz + rec->psiz;
    rec->kbuf = NULL;
    rec->vbuf = NULL;
    rec->boff = rec->off + hsiz;
    rec->bbuf = NULL;
    if (rsiz >= rec->ksiz) {
      rec->kbuf = rp;
      rp += rec->ksiz;
      rsiz -= rec->ksiz;
      if (rsiz >= rec->vsiz) {
        rec->vbuf = rp;
        if (rec->psiz > 0) {
          rp += rec->vsiz;
          rsiz -= rec->vsiz;
          if (rsiz > 0 && *(uint8_t*)rp != PADMAGIC) {
            set_error(_KCCODELINE_, Error::BROKEN, "invalid magic data of a record");
            report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld"
                   " snum=%04X", (long long)psiz_, (long long)rec->off, (long long)rsiz,
                   (long long)file_.size(), snum);
            report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rsiz);
            return false;
          }
        }
      }
    } else {
      if (rec->off + (int64_t)rec->rsiz > psiz_) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid length of a record");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld"
               " snum=%04X", (long long)psiz_, (long long)rec->off, (long long)rec->rsiz,
               (long long)file_.size(), snum);
        return false;
      }
      if (!read_record_body(rec)) return false;
    }
    return true;
  }
  /**
   * Read the body of a record from the file.
   * @param rec the record structure.
   * @return true on success, or false on failure.
   */
  bool read_record_body(Record* rec) {
    _assert_(rec);
    size_t bsiz = rec->ksiz + rec->vsiz;
    if (rec->psiz > 0) bsiz++;
    char* bbuf = new char[bsiz];
    if (!file_.read_fast(rec->boff, bbuf, bsiz)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)rec->boff, (long long)file_.size());
      delete[] bbuf;
      return false;
    }
    if (rec->psiz > 0 && ((uint8_t*)bbuf)[bsiz-1] != PADMAGIC) {
      set_error(_KCCODELINE_, Error::BROKEN, "invalid magic data of a record");
      report_binary(_KCCODELINE_, Logger::WARN, "bbuf", bbuf, bsiz);
      delete[] bbuf;
      return false;
    }
    rec->bbuf = bbuf;
    rec->kbuf = rec->bbuf;
    rec->vbuf = rec->bbuf + rec->ksiz;
    return true;
  }
  /**
   * Write a record into the file.
   * @param rec the record structure.
   * @param over true for overwriting, or false for new record.
   * @return true on success, or false on failure.
   */
  bool write_record(Record* rec, bool over) {
    _assert_(rec);
    char stack[IOBUFSIZ];
    char* rbuf = rec->rsiz > sizeof(stack) ? new char[rec->rsiz] : stack;
    char* wp = rbuf;
    uint16_t snum = hton16(rec->psiz);
    std::memcpy(wp, &snum, sizeof(snum));
    if (rec->psiz < 0x100) *wp = RECMAGIC;
    wp += sizeof(snum);
    writefixnum(wp, rec->left >> apow_, width_);
    wp += width_;
    if (!linear_) {
      writefixnum(wp, rec->right >> apow_, width_);
      wp += width_;
    }
    wp += writevarnum(wp, rec->ksiz);
    wp += writevarnum(wp, rec->vsiz);
    std::memcpy(wp, rec->kbuf, rec->ksiz);
    wp += rec->ksiz;
    std::memcpy(wp, rec->vbuf, rec->vsiz);
    wp += rec->vsiz;
    if (rec->psiz > 0) {
      std::memset(wp, 0, rec->psiz);
      *wp = PADMAGIC;
      wp += rec->psiz;
    }
    bool err = false;
    if (over) {
      if (!file_.write_fast(rec->off, rbuf, rec->rsiz)) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        err = true;
      }
    } else {
      if (!file_.write(rec->off, rbuf, rec->rsiz)) {
        set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
        err = true;
      }
    }
    if (rbuf != stack) delete[] rbuf;
    return !err;
  }
  /**
   * Adjust the padding of a record.
   * @param rec the record structure.
   * @return true on success, or false on failure.
   */
  bool adjust_record(Record* rec) {
    _assert_(rec);
    if (rec->psiz > (size_t)INT16MAX || rec->psiz > rec->rsiz / 2) {
      size_t nsiz = (rec->psiz >> apow_) << apow_;
      if (nsiz < rhsiz_) return true;
      rec->rsiz -= nsiz;
      rec->psiz -= nsiz;
      int64_t noff = rec->off + rec->rsiz;
      char nbuf[RECBUFSIZ];
      if (!write_free_block(noff, nsiz, nbuf)) return false;
      insert_free_block(noff, nsiz);
    }
    return true;
  }
  /**
   * Calculate the size of a record.
   * @param ksiz the size of the key.
   * @param vsiz the size of the value.
   * @return the size of the record.
   */
  size_t calc_record_size(size_t ksiz, size_t vsiz) {
    _assert_(true);
    size_t rsiz = sizeof(uint16_t) + width_;
    if (!linear_) rsiz += width_;
    if (ksiz < (1ULL << 7)) {
      rsiz += 1;
    } else if (ksiz < (1ULL << 14)) {
      rsiz += 2;
    } else if (ksiz < (1ULL << 21)) {
      rsiz += 3;
    } else if (ksiz < (1ULL << 28)) {
      rsiz += 4;
    } else {
      rsiz += 5;
    }
    if (vsiz < (1ULL << 7)) {
      rsiz += 1;
    } else if (vsiz < (1ULL << 14)) {
      rsiz += 2;
    } else if (vsiz < (1ULL << 21)) {
      rsiz += 3;
    } else if (vsiz < (1ULL << 28)) {
      rsiz += 4;
    } else {
      rsiz += 5;
    }
    rsiz += ksiz;
    rsiz += vsiz;
    return rsiz;
  }
  /**
   * Calculate the padding size of a record.
   * @param rsiz the size of the record.
   * @return the size of the padding.
   */
  size_t calc_record_padding(size_t rsiz) {
    _assert_(true);
    size_t diff = rsiz & (align_ - 1);
    return diff > 0 ? align_ - diff : 0;
  }
  /**
   * Shift a record to another place.
   * @param orec the original record structure.
   * @param dest the destination offset.
   * @return true on success, or false on failure.
   */
  bool shift_record(Record* orec, int64_t dest) {
    _assert_(orec && dest >= 0);
    uint64_t hash = hash_record(orec->kbuf, orec->ksiz);
    uint32_t pivot = fold_hash(hash);
    int64_t bidx = hash % bnum_;
    int64_t off = get_bucket(bidx);
    if (off < 0) return false;
    if (off == orec->off) {
      orec->off = dest;
      if (!write_record(orec, true)) return false;
      if (!set_bucket(bidx, dest)) return false;
      return true;
    }
    int64_t entoff = 0;
    Record rec;
    char rbuf[RECBUFSIZ];
    while (off > 0) {
      rec.off = off;
      if (!read_record(&rec, rbuf)) return false;
      if (rec.psiz == UINT16MAX) {
        set_error(_KCCODELINE_, Error::BROKEN, "free block in the chain");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
               (long long)psiz_, (long long)rec.off, (long long)file_.size());
        return false;
      }
      uint32_t tpivot = linear_ ? pivot : fold_hash(hash_record(rec.kbuf, rec.ksiz));
      if (pivot > tpivot) {
        delete[] rec.bbuf;
        off = rec.left;
        entoff = rec.off + sizeof(uint16_t);
      } else if (pivot < tpivot) {
        delete[] rec.bbuf;
        off = rec.right;
        entoff = rec.off + sizeof(uint16_t) + width_;
      } else {
        int32_t kcmp = compare_keys(orec->kbuf, orec->ksiz, rec.kbuf, rec.ksiz);
        if (linear_ && kcmp != 0) kcmp = 1;
        if (kcmp > 0) {
          delete[] rec.bbuf;
          off = rec.left;
          entoff = rec.off + sizeof(uint16_t);
        } else if (kcmp < 0) {
          delete[] rec.bbuf;
          off = rec.right;
          entoff = rec.off + sizeof(uint16_t) + width_;
        } else {
          delete[] rec.bbuf;
          orec->off = dest;
          if (!write_record(orec, true)) return false;
          if (entoff > 0) {
            if (!set_chain(entoff, dest)) return false;
          } else {
            if (!set_bucket(bidx, dest)) return false;
          }
          return true;
        }
      }
    }
    set_error(_KCCODELINE_, Error::BROKEN, "no record to shift");
    report(_KCCODELINE_, Logger::WARN, "psiz=%lld fsiz=%lld",
           (long long)psiz_, (long long)file_.size());
    return false;
  }
  /**
   * Write a free block into the file.
   * @param off the offset of the free block.
   * @param rsiz the size of the free block.
   * @param rbuf the working buffer.
   * @return true on success, or false on failure.
   */
  bool write_free_block(int64_t off, size_t rsiz, char* rbuf) {
    _assert_(off >= 0 && rbuf);
    char* wp = rbuf;
    *(wp++) = FBMAGIC;
    *(wp++) = FBMAGIC;
    writefixnum(wp, rsiz >> apow_, width_);
    wp += width_;
    *(wp++) = PADMAGIC;
    *(wp++) = PADMAGIC;
    if (!file_.write_fast(off, rbuf, wp - rbuf)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    return true;
  }
  /**
   * Insert a free block to the free block pool.
   * @param off the offset of the free block.
   * @param rsiz the size of the free block.
   */
  void insert_free_block(int64_t off, size_t rsiz) {
    _assert_(off >= 0);
    ScopedMutex lock(&flock_);
    escape_cursors(off, off + rsiz);
    if (fbpnum_ < 1) return;
    if (fbp_.size() >= (size_t)fbpnum_) {
      FBP::const_iterator it = fbp_.begin();
      if (rsiz <= it->rsiz) return;
      fbp_.erase(it);
    }
    FreeBlock fb = { off, rsiz };
    fbp_.insert(fb);
  }
  /**
   * Fetch the free block pool from a decent sized block.
   * @param rsiz the minimum size of the block.
   * @param res the structure for the result.
   * @return true on success, or false on failure.
   */
  bool fetch_free_block(size_t rsiz, FreeBlock* res) {
    _assert_(res);
    if (fbpnum_ < 1) return false;
    ScopedMutex lock(&flock_);
    FreeBlock fb = { INT64MAX, rsiz };
    FBP::const_iterator it = fbp_.upper_bound(fb);
    if (it == fbp_.end()) return false;
    res->off = it->off;
    res->rsiz = it->rsiz;
    fbp_.erase(it);
    escape_cursors(res->off, res->off + res->rsiz);
    return true;
  }
  /**
   * Trim invalid free blocks.
   * @param begin the beginning offset.
   * @param end the end offset.
   */
  void trim_free_blocks(int64_t begin, int64_t end) {
    _assert_(begin >= 0 && end >= 0);
    FBP::const_iterator it = fbp_.begin();
    FBP::const_iterator itend = fbp_.end();
    while (it != itend) {
      if (it->off >= begin && it->off < end) {
        fbp_.erase(it++);
      } else {
        ++it;
      }
    }
  }
  /**
   * Dump all free blocks into the file.
   * @return true on success, or false on failure.
   */
  bool dump_free_blocks() {
    _assert_(true);
    if (fbpnum_ < 1) return true;
    size_t size = boff_ - HEADSIZ;
    char* rbuf = new char[size];
    char* wp = rbuf;
    char* end = rbuf + size - width_ * 2 - sizeof(uint8_t) * 2;
    size_t num = fbp_.size();
    if (num > 0) {
      FreeBlock* blocks = new FreeBlock[num];
      size_t cnt = 0;
      FBP::const_iterator it = fbp_.begin();
      FBP::const_iterator itend = fbp_.end();
      while (it != itend) {
        blocks[cnt++] = *it;
        ++it;
      }
      std::sort(blocks, blocks + num, FreeBlockComparator());
      for (size_t i = num - 1; i > 0; i--) {
        blocks[i].off -= blocks[i-1].off;
      }
      for (size_t i = 0; wp < end && i < num; i++) {
        wp += writevarnum(wp, blocks[i].off >> apow_);
        wp += writevarnum(wp, blocks[i].rsiz >> apow_);
      }
      delete[] blocks;
    }
    *(wp++) = 0;
    *(wp++) = 0;
    bool err = false;
    if (!file_.write(HEADSIZ, rbuf, wp - rbuf)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    delete[] rbuf;
    return !err;
  }
  /**
   * Dump an empty set of free blocks into the file.
   * @return true on success, or false on failure.
   */
  bool dump_empty_free_blocks() {
    _assert_(true);
    if (fbpnum_ < 1) return true;
    char rbuf[2];
    char* wp = rbuf;
    *(wp++) = 0;
    *(wp++) = 0;
    bool err = false;
    if (!file_.write(HEADSIZ, rbuf, wp - rbuf)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    return !err;
  }
  /**
   * Load all free blocks from from the file.
   * @return true on success, or false on failure.
   */
  bool load_free_blocks() {
    _assert_(true);
    if (fbpnum_ < 1) return true;
    size_t size = boff_ - HEADSIZ;
    char* rbuf = new char[size];
    if (!file_.read(HEADSIZ, rbuf, size)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
             (long long)psiz_, (long long)HEADSIZ, (long long)file_.size());
      delete[] rbuf;
      return false;
    }
    const char* rp = rbuf;
    FreeBlock* blocks = new FreeBlock[fbpnum_];
    int32_t num = 0;
    while (num < fbpnum_ && size > 1 && *rp != '\0') {
      uint64_t off;
      size_t step = readvarnum(rp, size, &off);
      if (step < 1 || off < 1) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid free block offset");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
               (long long)psiz_, (long long)off, (long long)file_.size());
        delete[] rbuf;
        delete[] blocks;
        return false;
      }
      rp += step;
      size -= step;
      uint64_t rsiz;
      step = readvarnum(rp, size, &rsiz);
      if (step < 1 || rsiz < 1) {
        set_error(_KCCODELINE_, Error::BROKEN, "invalid free block size");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld rsiz=%lld fsiz=%lld",
               (long long)psiz_, (long long)off, (long long)rsiz, (long long)file_.size());
        delete[] rbuf;
        delete[] blocks;
        return false;
      }
      rp += step;
      size -= step;
      blocks[num].off = off << apow_;
      blocks[num].rsiz = rsiz << apow_;
      num++;
    }
    for (int32_t i = 1; i < num; i++) {
      blocks[i].off += blocks[i-1].off;
    }
    for (int32_t i = 0; i < num; i++) {
      FreeBlock fb = { blocks[i].off, blocks[i].rsiz };
      fbp_.insert(fb);
    }
    delete[] blocks;
    delete[] rbuf;
    return true;
  }
  /**
   * Disable all cursors.
   */
  void disable_cursors() {
    _assert_(true);
    if (curs_.empty()) return;
    CursorList::const_iterator cit = curs_.begin();
    CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      cur->off_ = 0;
      ++cit;
    }
  }
  /**
   * Escape cursors on a free block.
   * @param off the offset of the free block.
   * @param dest the destination offset.
   */
  void escape_cursors(int64_t off, int64_t dest) {
    _assert_(off >= 0 && dest >= 0);
    if (curs_.empty()) return;
    CursorList::const_iterator cit = curs_.begin();
    CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      if (cur->end_ == off) {
        cur->end_ = dest;
        if (cur->off_ >= cur->end_) cur->off_ = 0;
      }
      if (cur->off_ == off) {
        cur->off_ = dest;
        if (cur->off_ >= cur->end_) cur->off_ = 0;
      }
      ++cit;
    }
  }
  /**
   * Trim invalid cursors.
   */
  void trim_cursors() {
    _assert_(true);
    if (curs_.empty()) return;
    int64_t end = lsiz_;
    CursorList::const_iterator cit = curs_.begin();
    CursorList::const_iterator citend = curs_.end();
    while (cit != citend) {
      Cursor* cur = *cit;
      if (cur->off_ >= end) {
        cur->off_ = 0;
      } else if (cur->end_ > end) {
        cur->end_ = end;
      }
      ++cit;
    }
  }
  /**
   * Remove a record from a bucket chain.
   * @param rec the record structure.
   * @param rbuf the working buffer.
   * @param bidx the bucket index.
   * @param entoff the offset of the entry pointer.
   * @return true on success, or false on failure.
   */
  bool cut_chain(Record* rec, char* rbuf, int64_t bidx, int64_t entoff) {
    _assert_(rec && rbuf && bidx >= 0 && entoff >= 0);
    int64_t child;
    if (rec->left > 0 && rec->right < 1) {
      child = rec->left;
    } else if (rec->left < 1 && rec->right > 0) {
      child = rec->right;
    } else if (rec->left < 1) {
      child = 0;
    } else {
      Record prec;
      prec.off = rec->left;
      if (!read_record(&prec, rbuf)) return false;
      if (prec.psiz == UINT16MAX) {
        set_error(_KCCODELINE_, Error::BROKEN, "free block in the chain");
        report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
               (long long)psiz_, (long long)prec.off, (long long)file_.size());
        report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rhsiz_);
        return false;
      }
      delete[] prec.bbuf;
      if (prec.right > 0) {
        int64_t off = prec.right;
        int64_t pentoff = prec.off + sizeof(uint16_t) + width_;
        while (true) {
          prec.off = off;
          if (!read_record(&prec, rbuf)) return false;
          if (prec.psiz == UINT16MAX) {
            set_error(_KCCODELINE_, Error::BROKEN, "free block in the chain");
            report(_KCCODELINE_, Logger::WARN, "psiz=%lld off=%lld fsiz=%lld",
                   (long long)psiz_, (long long)prec.off, (long long)file_.size());
            report_binary(_KCCODELINE_, Logger::WARN, "rbuf", rbuf, rhsiz_);
            return false;
          }
          delete[] prec.bbuf;
          if (prec.right < 1) break;
          off = prec.right;
          pentoff = prec.off + sizeof(uint16_t) + width_;
        }
        child = off;
        if (!set_chain(pentoff, prec.left)) return false;
        if (!set_chain(off + sizeof(uint16_t), rec->left)) return false;
        if (!set_chain(off + sizeof(uint16_t) + width_, rec->right)) return false;
      } else {
        child = prec.off;
        if (!set_chain(prec.off + sizeof(uint16_t) + width_, rec->right)) return false;
      }
    }
    if (entoff > 0) {
      if (!set_chain(entoff, child)) return false;
    } else {
      if (!set_bucket(bidx, child)) return false;
    }
    return true;
  }
  /**
   * Begin transaction.
   * @return true on success, or false on failure.
   */
  bool begin_transaction_impl() {
    _assert_(true);
    if ((count_ != trcount_ || lsiz_ != trsize_) && !dump_meta()) return false;
    if (!file_.begin_transaction(trhard_, boff_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      return false;
    }
    if (!file_.write_transaction(MOFFBNUM, HEADSIZ - MOFFBNUM)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      file_.end_transaction(false);
      return false;
    }
    if (fbpnum_ > 0) {
      FBP::const_iterator it = fbp_.end();
      FBP::const_iterator itbeg = fbp_.begin();
      for (int32_t cnt = fpow_ * 2 + 1; cnt > 0; cnt--) {
        if (it == itbeg) break;
        --it;
        trfbp_.insert(*it);
      }
    }
    return true;
  }
  /**
   * Begin auto transaction.
   * @return true on success, or false on failure.
   */
  bool begin_auto_transaction() {
    _assert_(true);
    atlock_.lock();
    if (!file_.begin_transaction(autosync_, boff_)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      atlock_.unlock();
      return false;
    }
    if (!file_.write_transaction(MOFFCOUNT, MOFFOPAQUE - MOFFCOUNT)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      file_.end_transaction(false);
      atlock_.unlock();
      return false;
    }
    return true;
  }
  /**
   * Commit transaction.
   * @return true on success, or false on failure.
   */
  bool commit_transaction() {
    _assert_(true);
    bool err = false;
    if ((count_ != trcount_ || lsiz_ != trsize_) && !dump_auto_meta()) err = true;
    if (!file_.end_transaction(true)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    trfbp_.clear();
    return !err;
  }
  /**
   * Commit auto transaction.
   * @return true on success, or false on failure.
   */
  bool commit_auto_transaction() {
    _assert_(true);
    bool err = false;
    if ((count_ != trcount_ || lsiz_ != trsize_) && !dump_auto_meta()) err = true;
    if (!file_.end_transaction(true)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    atlock_.unlock();
    return !err;
  }
  /**
   * Abort transaction.
   * @return true on success, or false on failure.
   */
  bool abort_transaction() {
    _assert_(true);
    bool err = false;
    if (!file_.end_transaction(false)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    bool flagopen = flagopen_;
    if (!load_meta()) err = true;
    flagopen_ = flagopen;
    calc_meta();
    disable_cursors();
    fbp_.swap(trfbp_);
    trfbp_.clear();
    return !err;
  }
  /**
   * Abort auto transaction.
   * @return true on success, or false on failure.
   */
  bool abort_auto_transaction() {
    _assert_(true);
    bool err = false;
    if (!file_.end_transaction(false)) {
      set_error(_KCCODELINE_, Error::SYSTEM, file_.error());
      err = true;
    }
    if (!load_meta()) err = true;
    calc_meta();
    disable_cursors();
    fbp_.clear();
    atlock_.unlock();
    return !err;
  }
  /** Dummy constructor to forbid the use. */
  HashDB(const HashDB&);
  /** Dummy Operator to forbid the use. */
  HashDB& operator =(const HashDB&);
  /** The method lock. */
  RWLock mlock_;
  /** The record locks. */
  SlottedRWLock rlock_;
  /** The file lock. */
  Mutex flock_;
  /** The auto transaction lock. */
  Mutex atlock_;
  /** The last happened error. */
  TSD<Error> error_;
  /** The internal logger. */
  Logger* logger_;
  /** The kinds of logged messages. */
  uint32_t logkinds_;
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
  /** The flag for reorganized. */
  bool reorg_;
  /** The flag for trimmed. */
  bool trim_;
  /** The file for data. */
  File file_;
  /** The free block pool. */
  FBP fbp_;
  /** The cursor objects. */
  CursorList curs_;
  /** The path of the database file. */
  std::string path_;
  /** The library version. */
  uint8_t libver_;
  /** The library revision. */
  uint8_t librev_;
  /** The format revision. */
  uint8_t fmtver_;
  /** The module checksum. */
  uint8_t chksum_;
  /** The database type. */
  uint8_t type_;
  /** The alignment power. */
  uint8_t apow_;
  /** The free block pool power. */
  uint8_t fpow_;
  /** The options. */
  uint8_t opts_;
  /** The bucket number. */
  int64_t bnum_;
  /** The status flags. */
  uint8_t flags_;
  /** The flag for open. */
  bool flagopen_;
  /** The record number. */
  AtomicInt64 count_;
  /** The logical size of the file. */
  AtomicInt64 lsiz_;
  /** The physical size of the file. */
  AtomicInt64 psiz_;
  /** The opaque data. */
  char opaque_[HEADSIZ-MOFFOPAQUE];
  /** The size of the internal memory-mapped region. */
  int64_t msiz_;
  /** The unit step number of auto defragmentation. */
  int64_t dfunit_;
  /** The embedded data compressor. */
  Compressor* embcomp_;
  /** The alignment of records. */
  size_t align_;
  /** The number of elements of the free block pool. */
  int32_t fbpnum_;
  /** The width of record addressing. */
  int32_t width_;
  /** The flag for linear collision chaining. */
  bool linear_;
  /** The data compressor. */
  Compressor* comp_;
  /** The header size of a record. */
  size_t rhsiz_;
  /** The offset of the buckets section. */
  int64_t boff_;
  /** The offset of the record section. */
  int64_t roff_;
  /** The defrag cursor. */
  int64_t dfcur_;
  /** The count of fragmentation. */
  AtomicInt64 frgcnt_;
  /** The flag whether in transaction. */
  bool tran_;
  /** The flag whether hard transaction. */
  bool trhard_;
  /** The escaped free block pool for transaction. */
  FBP trfbp_;
  /** The count history for transaction. */
  int64_t trcount_;
  /** The size history for transaction. */
  int64_t trsize_;
};


/** An alias of the file tree database. */
typedef PlantDB<HashDB, BasicDB::TYPETREE> TreeDB;


}                                        // common namespace

#endif                                   // duplication check

// END OF FILE
